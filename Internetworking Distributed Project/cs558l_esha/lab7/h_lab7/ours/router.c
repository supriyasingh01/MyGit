#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <pcap.h>
#define MAX_HOSTS 10
#include <stdlib.h>
#include <netinet/if_ether.h> /* includes net/ethernet.h */
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <features.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include<net/if.h>

#define MAX_PACKET_SIZE 5000
#define Entries 10
#define NUM_THREADS 10
/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14
#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)
	#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)


// Global data variables

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};



/* IP header */
	struct sniff_ip {
		u_char ip_vhl;		/* version << 4 | header length >> 2 */
		u_char ip_tos;		/* type of service */
		u_short ip_len;		/* total length */
		u_short ip_id;		/* identification */
		u_short ip_off;		/* fragment offset field */
	#define IP_RF 0x8000		/* reserved fragment flag */
	#define IP_DF 0x4000		/* dont fragment flag */
	#define IP_MF 0x2000		/* more fragments flag */
	#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
		u_char ip_ttl;		/* time to live */
		u_char ip_p;		/* protocol */
		u_short ip_sum;		/* checksum */
		struct in_addr ip_src,ip_dst; /* source and dest address */
	};

int entrycnt=-1;
struct Rtable
{ 
	bpf_u_int32 mask;	//mask for next hop IP
	bpf_u_int32 dest;	//destination network add
	bpf_u_int32 next;	//next hop router IP
	char interface[10]; //interface number
	u_char mac[6];
	int interfaceposn;
} router_table[Entries];

struct arptable
{
	bpf_u_int32 destn;
	u_char macaddr[6];
	char interface[10];
};

int num=0;
struct arptable arp_table[MAX_HOSTS];
int arpentrycnt=-1;

struct interface
{
	char name[10];
	u_char mac[6];
	
}eth_int[5];

void got_packet( u_char *packet,char *,int,pcap_t *);
void interfacedetail(char * name);


void display_arptable()
{
	int i;
	printf("Displaying ARP table\n");
	printf("===================================================\n");

	printf("Total ARP entries : %d\n",arpentrycnt);
	for(i=0;i<=arpentrycnt;i++)
	{
		printf("\n destn=%u macaddr=%x:%x:%x:%x:%x:%x interface=%s",arp_table[i].destn,\
			arp_table[i].macaddr[0],arp_table[i].macaddr[1],arp_table[i].macaddr[2],\
			arp_table[i].macaddr[3],arp_table[i].macaddr[4],arp_table[i].macaddr[5],arp_table[i].interface);
	}

	printf("===================================================\n");

}

void arpquery(char *mydev,char * myhost)
{
	char cmd[512];
	bzero(cmd,sizeof(cmd));
	snprintf(cmd,sizeof(cmd),"sudo /usr/bin/arping %s -I %s -c 1 -q",myhost,mydev);
	printf("\ncmd=%s",cmd);
	system(cmd);
}	

//type:rtr or arp
//locn:where in rtr
populate_arp(char *hostIP,char *mydev,int type,int locn)
{
	int                 s;
	struct arpreq       areq;
	struct sockaddr_in *sin;
	struct in_addr      ipaddr;
	int var=0;	
	
	printf("\n\n[debug] Populate ARP for %s\n\n",mydev);
	if(strlen(mydev) == 0)
		return;
		
		
	/* Get an internet domain socket. */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	char buffer_mac[256];
	/* Make the ARP request. */
	memset(&areq, 0, sizeof(areq));
	sin = (struct sockaddr_in *) &areq.arp_pa;
	sin->sin_family = AF_INET;

	if (inet_aton(hostIP, &ipaddr) == 0) {
		fprintf(stderr, "-- Error: invalid numbers-and-dots IP address %s.\n",hostIP);
		exit(1);
	}
	
	sin->sin_addr = ipaddr; 			//I gave the IP to find the entry for
	sin = (struct sockaddr_in *) &areq.arp_ha;
	sin->sin_family = ARPHRD_ETHER;
	
	strncpy(areq.arp_dev,mydev, 15);
		
	while (ioctl(s, SIOCGARP, (caddr_t) &areq) == -1) {
		perror("-- Error: unable to make ARP request, error");
		arpquery(mydev,hostIP);
		if(var==10)
			exit(0);
		var++;
	}
	struct sockaddr * addr;
	addr=(struct sockaddr*)&areq.arp_ha;
	unsigned char *ptr = (unsigned char *)&addr->sa_data;
	
	
	if(type==1) //rtr	
	memcpy(router_table[locn].mac,ptr,6);
	else
	memcpy(arp_table[arpentrycnt].macaddr,ptr,6);
}

int arpentry(char * hostip, char *mydev, bpf_u_int32 ipaddr,int type,int locn)
{
	int i;
	for(i=0;i<=arpentrycnt;i++)
	{
		if(arp_table[i].destn==ipaddr)
		return i;
	}
	if(i>arpentrycnt)
	{
		if(type==0){
			++arpentrycnt;
			strcpy(arp_table[arpentrycnt].interface,mydev);
			arp_table[arpentrycnt].destn = ipaddr;
		}
		populate_arp(hostip,mydev,type,locn);
	}	
	return i;
}

int getarpentry(bpf_u_int32 ipaddr)
{
	int i;
	for(i=0;i<=arpentrycnt;i++)
	{
		if(arp_table[i].destn==ipaddr)
			return i;
	}
	return -1;
}

int comparedetail(bpf_u_int32 target)
{
	int i;
	for(i=0;i<=entrycnt;i++){
		if((target & router_table[i].mask) == router_table[i].dest) //destination matches with next hop ip
					return i;
	}
	return(i);
}



unsigned char ipmac1[25], ipmac2[25];
int maxinterfaceentry=-1;	

void getmacaddress(char *dev,int posn)
{
	int skfd=0,i;
	struct ifreq ifr;
	
	strcpy(ifr.ifr_name,dev);
	skfd=socket(AF_INET,SOCK_DGRAM,0);
	if(ioctl(skfd,SIOCGIFHWADDR,&ifr)<0)
	{ 
		printf("%s-%d: [Error] Should not come here\n",__FUNCTION__,__LINE__);
	}
	bzero(eth_int[posn].mac,6);
		
	unsigned char *p=ifr.ifr_hwaddr.sa_data;
	for(i=0;i<6;i++)
		eth_int[posn].mac[i]=p[i] & 0377;
	printf("%s-%d: Printing macs for interface: %s\n",__FUNCTION__,__LINE__,dev);
	if(posn==0)
	{
		sprintf(ipmac1,"%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",p[0],p[1],p[2],p[3],p[4],p[5]);
	    printf("%.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",p[0],p[1],p[2],p[3],p[4],p[5]);
	}
	else
	{
		sprintf(ipmac2,"%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",p[0],p[1],p[2],p[3],p[4],p[5]);
		printf("%.02x:%.02x:%.02x:%.02x:%.02x:%.02x\n",p[0],p[1],p[2],p[3],p[4],p[5]);

	}
	close(skfd);
}

// Function keeps track of each interface local to router
int interfaceentry(char * interface)
{
	int i;

	for(i=0;i<=maxinterfaceentry;i++)
	{
		if(strcmp(interface,eth_int[i].name)==0)			// Interface already exists in eth_int, just return it's index
			return i;
	}
	
	if(i>maxinterfaceentry)							// Got a new interface
	{
		++maxinterfaceentry;								// Tracks total number of interfaces
		strcpy(eth_int[maxinterfaceentry].name,interface);	// Save it's name to eth_int
		getmacaddress(interface,maxinterfaceentry);			// Get its mac address
	}
	return i;
}

void display_interface() 		//displays name and mac
{
	int i;
	printf("Displaying local interfaces\n");
	printf("===================================================\n");
	for(i=0;i<=maxinterfaceentry;i++)
	{
		printf("%d || %s || %x:%x:%x:%x:%x:%x\n",i,eth_int[i].name,eth_int[i].mac[0],eth_int[i].mac[1],eth_int[i].mac[2],eth_int[i].mac[3],eth_int[i].mac[4],eth_int[i].mac[5]);
	}	
	printf("===================================================\n");

}	


void display_rtr()
{
	int i=0;
	printf("Displaying Router Table\n");
	printf("====================================================\n");
	printf("Dest	  Next-Hop	Mask 	Interface	Type\n");
	printf("====================================================\n");
	for(i=0;i<=entrycnt;i++)
	{
		//printf("Printing entry :: %d\n",i);
		printf("%s  ",(char *)inet_ntoa(router_table[i].dest));
		printf("%s  ",(char *)inet_ntoa(router_table[i].next));
		printf("%s  %s  \t%d\n",(char *)inet_ntoa(router_table[i].mask),router_table[i].interface,router_table[i].interfaceposn);
		if(router_table[i].next!=0)
		{
			printf("\nMAC=%x:%x:%x:%x:%x:%x\n",router_table[i].mac[0],router_table[i].mac[1],router_table[i].mac[2],router_table[i].mac[3],router_table[i].mac[4],router_table[i].mac[5]);
		}
		//else
		//	printf("\nnot found in arp table");
		
		printf("===================================================\n");
	}	
}		

void read_config(char *myconfig)
{
	FILE *cfg;
	cfg=fopen(myconfig,"rt");		// Open
	char buf[50],buf1[50];
	
	bzero(buf,sizeof(buf));
	bzero(buf1,sizeof(buf1));
	
	// 10.1.2.0 10.99.0.2 255.255.254.0 eth0 
	while(!feof(cfg))
	{
		fscanf(cfg,"%s",buf);		
		if(feof(cfg))
			break;	
			
		++entrycnt;

		inet_aton(buf,&router_table[entrycnt].dest);			// Destination  10.1.2.0
	
		fscanf(cfg,"%s",buf1);
		inet_aton(buf1,&router_table[entrycnt].next);			// Next Hop  10.99.0.2
		
		fscanf(cfg,"%s",buf);
		inet_aton(buf,&router_table[entrycnt].mask);			// Mask   255.255.254.0
		fscanf(cfg,"%s",router_table[entrycnt].interface);		// Interface  eth0 
		
		
		
		router_table[entrycnt].interfaceposn = interfaceentry(router_table[entrycnt].interface);		// Interface position
		
		if(router_table[entrycnt].next != 0)					// Get MAC of next hop   //not local address
			arpentry(buf1,router_table[entrycnt].interface,router_table[entrycnt].next,1,entrycnt);			// Go and fetch mac of that interface
		
	}
	bzero(buf,sizeof(buf));
	fclose(cfg);

}

pcap_t *global_desc[2];
int main(int argc, char **argv) 
{
    int pthreadcnt=0;
    pthread_t threads[NUM_THREADS];
    pcap_if_t *alldevs, *d;
    pcap_t *fp;
    u_int inum, i=0;
    char errbuf[PCAP_ERRBUF_SIZE];
    int res;
    struct pcap_pkthdr *header;
    u_char *pkt_data;
    char *dev; /* name of the device to use */ 
    void * status;
 
    int cnt=0; 

   	if (argc < 2 || argc > 2) {
		fprintf(stderr, "-- Usage: %s config_file\n", argv[0]);
		exit(1);
	}
	
	read_config(argv[1]);
	// You have all interfaces, their macs and built the routing table 
	
	display_rtr();	
	display_interface();
	//display_arptable();
    
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
            fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
            exit(1);
    }
    
    // Loops twice for both interface. Opens pcap session on these interfaces 
    // so that you can later inject packets
	for(i=0;i<2;i++)			
	{
		printf("Opening pcap session for %s Interface Position: %d\n",eth_int[i].name, i);
		global_desc[i] = pcap_open_live(eth_int[i].name,BUFSIZ,0,-1,errbuf);
		if(global_desc[i] == NULL){
			printf("%s-%d [Error] Could not open pcap_open_live %s\n",__FUNCTION__,__LINE__,errbuf);
			exit(0); 
		}
	}
	
	
	
	
	
	for(i=0;i<=maxinterfaceentry;i++)			// for each interface
	{
		int rc;
		printf("Creating pthread for interface : %s\n",eth_int[i].name);
	    rc= pthread_create(&threads[i], NULL, (void *)interfacedetail, eth_int[i].name);
    }	
	pthread_exit(NULL);
	
	
	return 1;

}

u_char arr[6];
void interfacedetail(char * dev)
{
	int ret;   /* return code */
	char *net; /* dot notation of the network address */
    char *mask;/* dot notation of the network mask    */
    char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 netp; /* ip          */
    bpf_u_int32 maskp;/* subnet mask */
    struct in_addr addr;
	pcap_t* descr;
	u_char *packet;
	struct pcap_pkthdr hdr;     /* pcap.h */
    struct ether_header *eptr;  /* net/ethernet.h */
	struct bpf_program fp;

	packet=(u_char *) malloc( sizeof(MAX_PACKET_SIZE));
    
    // Finds network properties of that interface
	ret = pcap_lookupnet(dev,&netp,&maskp,errbuf);		
	if(ret == -1)
	{
		printf("%s\n",errbuf);
		exit(1);
	}

	/* get the network address in a human readable form */
	addr.s_addr = netp;
	net = (char *)inet_ntoa(addr);
	if(net == NULL)
	{
		perror("inet_ntoa");
		exit(1);
	}

	/* do the same as above for the device's mask */
	addr.s_addr = maskp;
	mask = (char *)inet_ntoa(addr);
    if(mask == NULL)
	{
		perror("inet_ntoa");
		exit(1);
	}
    
		
	/* Open session on 
		'dev'    : device
		'BUFSIZ' : maximum number of bytes to be captured
		'0'		 : Non-promiscuous mode
		'0'		 : Wait indefinately for packet, no timeout
		'errbuf' : Container for error string (if any)
	*/
    descr = pcap_open_live(dev,BUFSIZ, 0, 0, errbuf);			// eth0
    if(descr == NULL)
    {
    	printf("%s-%d [Error] Couldn't open device %s: %s\n",__FUNCTION__, __LINE__, dev, errbuf);
    }
    
    char filter_exp1[100];
	strcpy(filter_exp1,"ip and not ether src ");
	strcat(filter_exp1,ipmac1);			// "ip and not ether src <ipmac1>" RULE 1     
	strcat(filter_exp1," and not ether src "); // "and not ether src <ipmac2>" RULE 2
	strcat(filter_exp1,ipmac2);
	
	// filter_exp1 has the following 3 rules in it
	// 1. should be ip packet
	// 2. source mac should NOT be ipmac1
	// 3. source mac should NOT be ipmac2
	if(pcap_compile(descr, &fp, filter_exp1, 0, netp)==-1)			// Compile this rule into descr
	{
		printf("error");
		exit(0);
	}
	if(pcap_setfilter(descr,&fp)==-1)
	{
		printf("error");
		exit(0);
	}
 	
    if(descr == NULL)
    {
		printf("\nThread Exiting %s",dev);
		pthread_exit(NULL);
        return;
    }
	
	while(1)			// while loop to capture packets
	{
		printf("Waiting for packet on %s...\n",dev);
		packet = (u_char *) pcap_next(descr,&hdr);			// Packet will be returned in "packet"
		if(packet == NULL)
		{
			printf("%s-%d [Error] Didn't grab packet: %s\n",__FUNCTION__, __LINE__, dev);
			printf("%s-%d [Error] Thread Exiting %s\n",__FUNCTION__, __LINE__, dev);
			pthread_exit(NULL);
			break;
		}
		
		// "packet" is a pointer to first byte of packet you received
		
		eptr = (struct ether_header *) packet;
		//eptr->ether_type = (u_short)(packet +12);
		
		/* check to see if we have an ip packet */
		if (ntohs(eptr->ether_type) == ETHERTYPE_IP)
		{
			printf("===================================================\n");
			printf("Got IP packet on %s\n",dev);
			printf("===================================================\n");
			got_packet(packet, dev, hdr.len, descr);
		} 
	}
	printf("Thread Exiting %s",dev);
	pthread_exit(NULL);
}

/*
 * dissect/print packet
 */
 // Packet is just a raw stream of bytes
void got_packet( u_char *packet,char *dev,int len,pcap_t * descr)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	const struct sniff_ethernet *ethernet; 	/* The ethernet header */
	struct sniff_ip 		*ip; 		/* The IP header */
	const struct sniff_tcp 		*tcp; 		/* The TCP header */
	const char   				*payload; 	/* Packet payload */
	u_int size_ip;
	u_int size_tcp;
	struct in_addr ip_src, ip_dst;
	struct in_addr *ipd;
	u_int ip_ttl;
	
	ethernet = (struct sniff_ethernet*)(packet);		// Get ethernet header from raw packet
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);	// Get IP packet
	size_ip = IP_HL(ip)*4;								// Get size of ip header
	if (size_ip < 20) {
		printf("Invalid IP header length: %u bytes\n", size_ip);
		return;
	}
	
	ip_src = ip -> ip_src;
	ip_dst = ip -> ip_dst;
	ip_ttl = (u_int)ip -> ip_ttl;
	ip_ttl = ip_ttl - 1;
	bpf_u_int32 dst_input = ip_dst.s_addr;
    
	printf("IP size: %d\n",size_ip);
    printf("IP Source: %s\tDestination:%s\n",(char *)inet_ntoa(ip_src),(char *)inet_ntoa(ip_dst));
    
    //ip -> ip_ttl = ip_ttl;
    //printf("Changed TLV to %u\n",(u_int)ip-> ip_ttl);
    //memcpy(packet + SIZE_ETHERNET, (u_char *)ip, sizeof(struct sniff_ip));
	
    /*---------- Match for destination-----------------------------------------------*/
	int match_index;
	match_index = comparedetail(dst_input);		// Returns entry index from routing table if present
	if(match_index == -1)
	{
		printf("%s-%d No match for packet destination\n",__FUNCTION__, __LINE__);
		return;
	}
	printf("%s-%d Found a match at %d\n",__FUNCTION__, __LINE__,match_index);
	printf("%s-%d Send to destination network %s \n",__FUNCTION__, __LINE__,(char *)inet_ntoa(router_table[match_index].dest));
	printf("%s-%d Send using next hop : %s\n",__FUNCTION__, __LINE__,(char *)inet_ntoa(router_table[match_index].next));	
	
	/*---------- Prepare packet--------------------------------------------------------------*/
	if(router_table[match_index].next == 0)				// Destination on locally connected interface
	{
		printf("%s-%d Local send to final dest %s\n",__FUNCTION__, __LINE__, (char *)inet_ntoa(ip_dst));
		memcpy(packet+ETHER_ADDR_LEN, eth_int[router_table[match_index].interfaceposn].mac, ETHER_ADDR_LEN); // Write destination address
		int posn = arpentry((char *)inet_ntoa(ip_dst), router_table[match_index].interface, dst_input,0,0);
		memcpy(packet,arp_table[posn].macaddr, ETHER_ADDR_LEN);									// Write source mac address
	}
	else									// Non local destination
	{
		printf("%s-%d Remote send to final dest %s\n",__FUNCTION__, __LINE__, (char *)inet_ntoa(ip_dst));
		memcpy(packet, router_table[match_index].mac, ETHER_ADDR_LEN);										 // Write destination mac of next hop
		memcpy(packet + ETHER_ADDR_LEN, eth_int[router_table[match_index].interfaceposn].mac, ETHER_ADDR_LEN); // Write source mac of sending device
	}
	
	/*---------- Inject--------------------------------------------------------------*/
	int rr;
	rr=pcap_inject(global_desc[router_table[match_index].interfaceposn], packet, len);
	if(rr < 0)
	{
		printf("%s-%d [Error] packet inject failed\n",__FUNCTION__, __LINE__);
		exit(0);
	}
	return;
}
