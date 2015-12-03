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

#define NUM_THREADS 10
/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

void got_packet( u_char *packet,char *,int,pcap_t *);
void interfacedetail(char * name);


#define Entries 10
int entrycnt=-1;
struct Rtable
{ 
	bpf_u_int32 mask;	//mask for next hop IP
	bpf_u_int32 dest;	//destination network add
	bpf_u_int32 next;	//next hop router IP
	char interface[10]; //interface number
	u_char mac[6];
	int interfaceposn;
} Real[Entries];

struct arptable
{
	bpf_u_int32 destn;
	u_char macaddr[6];
	char interface[10];
};

int num=0;
struct arptable index1[MAX_HOSTS];
int arpentrycnt=-1;

void display_arptable()
{
	int i;
	printf("\narpentrycnt=%d",arpentrycnt);
	for(i=0;i<=arpentrycnt;i++)
		//why so many mac address need to be printed below????
		printf("\n destn=%u macaddr=%x:%x:%x:%x:%x:%x interface=%s",index1[i].destn,index1[i].macaddr[0],index1[i].macaddr[1],index1[i].macaddr[2],index1[i].macaddr[3],index1[i].macaddr[4],index1[i].macaddr[5],index1[i].interface);
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
	memcpy(Real[locn].mac,ptr,6);
	else
	memcpy(index1[arpentrycnt].macaddr,ptr,6);
}
int arpentry(char * hostip, char *mydev, bpf_u_int32 ipaddr,int type,int locn)
{
	int i;
	for(i=0;i<=arpentrycnt;i++)
	{
		if(index1[i].destn==ipaddr)
		return i;
	}
	if(i>arpentrycnt)
	{
		if(type==0){
		++arpentrycnt;
		strcpy(index1[arpentrycnt].interface,mydev);
		index1[arpentrycnt].destn = ipaddr;
		}populate_arp(hostip,mydev,type,locn);
	}	
	return i;
}
int getarpentry(bpf_u_int32 ipaddr)
{
	int i;
	for(i=0;i<=arpentrycnt;i++)
	{
		if(index1[i].destn==ipaddr)
			return i;
	}
	return -1;
}	
int comparedetail(bpf_u_int32 target)
{
	int i;
	for(i=0;i<=entrycnt;i++){
		if((target & Real[i].mask) == Real[i].dest) //destination matches with next hop ip
					return i;
	}
	return(i);
}

struct interface
{
	char name[10];
	u_char mac[6];
	
}eth_int[5];
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
	}
	bzero(eth_int[posn].mac,6);
		
	unsigned char *p=ifr.ifr_hwaddr.sa_data;
	for(i=0;i<6;i++)
	eth_int[posn].mac[i]=p[i] & 0377;
	
	if(posn==0)
		sprintf(ipmac1,"%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",p[0],p[1],p[2],p[3],p[4],p[5]);
	else
		sprintf(ipmac2,"%.02x:%.02x:%.02x:%.02x:%.02x:%.02x",p[0],p[1],p[2],p[3],p[4],p[5]);
	close(skfd);
	return 0;
}
int interfaceentry(char * interface)
{
	int i;
	for(i=0;i<=maxinterfaceentry;i++)
	{
		if(strcmp(interface,eth_int[i].name)==0)
		return i;
	}
	if(i>maxinterfaceentry)
	{
		++maxinterfaceentry;
		strcpy(eth_int[maxinterfaceentry].name,interface);
		getmacaddress(interface,maxinterfaceentry);
	}
	return i;
}

void display_interface()//displaying interfaces????why ?
{
	int i;
	for(i=0;i<=maxinterfaceentry;i++)
	{
		printf("\n%d. %s %x:%x:%x:%x:%x:%x",i,eth_int[i].name,eth_int[i].mac[0],eth_int[i].mac[1],eth_int[i].mac[2],eth_int[i].mac[3],eth_int[i].mac[4],eth_int[i].mac[5]);
	}	
}	


void read_config(char *myconfig)
{
	FILE *cfg;
	cfg=fopen(myconfig,"rt");
	char buf[50],buf1[50];
	
	bzero(buf,sizeof(buf));
	bzero(buf1,sizeof(buf1));
	while(!feof(cfg))
	{
		fscanf(cfg,"%s",buf);
		if(feof(cfg))
		break;	
		++entrycnt;

		inet_aton(buf,&Real[entrycnt].dest);
	
		fscanf(cfg,"%s",buf1);
		inet_aton(buf1,&Real[entrycnt].next);
		
		fscanf(cfg,"%s",buf);
		inet_aton(buf,&Real[entrycnt].mask);
		fscanf(cfg,"%s",Real[entrycnt].interface);
		Real[entrycnt].interfaceposn=interfaceentry(Real[entrycnt].interface);
		if(Real[entrycnt].next!=0)
		arpentry(buf1,Real[entrycnt].interface,Real[entrycnt].next,1,entrycnt);
		
	}
	bzero(buf,sizeof(buf));
	fclose(cfg);

}

void display_rtr()//router table ----mac add of dest, mac addr of next hop, mask , interface ,interfaceposition
{
	int i=0;
	for(i=0;i<=entrycnt;i++)
	{
		printf("\n%s  ",inet_ntoa(Real[i].dest));//real is in router table structure definition
		printf("%s  ",inet_ntoa(Real[i].next));
		printf("%s  %s  %d",inet_ntoa(Real[i].mask),Real[i].interface,Real[i].interfaceposn);
		if(Real[i].next!=0)
		{
			printf("\n\nMAc=%x:%x:%x:%x:%x:%x",Real[i].mac[0],Real[i].mac[1],Real[i].mac[2],Real[i].mac[3],Real[i].mac[4],Real[i].mac[5]);
		}
		else
		printf("\n\nnot found in arp table");
	}	
}		



pcap_t * desc[2];
int main(int argc, char **argv) 
{
    int pthreadcnt=0;
    pthread_t threads[NUM_THREADS];
    pcap_if_t *alldevs, *d;//interface declaration
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
	
	read_config(argv[1]);//read configuration file arp.conf
	display_rtr();//display rtr table
	display_interface();//display interface???
	display_arptable();//display arp table
	printf("table displayed");
    
	if (pcap_findalldevs(&alldevs, errbuf) == -1)//find devices to sniff on
    {
            fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
            exit(1);
    }
	for(i=0;i<2;i++)//open devices to start sniff
	{
		desc[i]=pcap_open_live(eth_int[i].name,BUFSIZ,0,-1,errbuf);
		if(desc[i]==NULL){
		printf("descr2");
		exit(0); }
	}
	  /* Print the list */
	for(i=0;i<=maxinterfaceentry;i++)//creating a thread for each interface
	{
		int rc;
	     rc= pthread_create(&threads[i],NULL,(void *)interfacedetail,eth_int[i].name);
    }	
	pthread_exit(NULL);

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
	
	packet=(u_char *) malloc( sizeof(5000));
    
	ret = pcap_lookupnet(dev,&netp,&maskp,errbuf);//lookupnet looks up for network address and mask in human non-readable form
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
    
	
	struct bpf_program fp;

    descr = pcap_open_live(dev,BUFSIZ,0,-1,errbuf);//open the device for sniffing
	char filter_exp1[100];
	strcpy(filter_exp1,"ip and not ether src ");//???
	strcat(filter_exp1,ipmac1);//????
	strcat(filter_exp1," and not ether src ");//???
	strcat(filter_exp1,ipmac2);//?????
	
	if(pcap_compile(descr,&fp,filter_exp1,0,net)==-1)
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
	
	while(1)
	{
		packet = pcap_next(descr,&hdr);
		if(packet == NULL)
		{
			printf("\nDidn't grab packet: %s\n",dev);
			printf("\nThread Exiting %s",dev);
			pthread_exit(NULL);
			break;
		}
		eptr = (struct ether_header *) packet;
		//eptr->ether_type = (u_short)(packet +12);
		/* check to see if we have an ip packet */
		if (ntohs (eptr->ether_type) == ETHERTYPE_IP)
		{
			got_packet(packet,dev,hdr.len,descr);
		} 
	}
	printf("\nThread Exiting %s",dev);
	pthread_exit(NULL);
}

/*
 * dissect/print packet
 */
void got_packet( u_char *packet,char *dev,int len,pcap_t * descr)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	struct in_addr ip_dst;
		
	struct in_addr *ipd;
	ipd=(struct in_addr*)(packet+30);
	ip_dst=*ipd;
    
	char * buffer;

	bpf_u_int32 input=ip_dst.s_addr;
	buffer=(char *)inet_ntoa(ip_dst);
       
	int match;
	match=comparedetail(input);
	if(match==-1)
	return;
	
	if(Real[match].next==0)
	{
		memcpy(packet+6,eth_int[Real[match].interfaceposn].mac,6);
		int posn=arpentry(buffer, Real[match].interface, input,0,0);
		memcpy(packet,index1[posn].macaddr,6);
	}
	else
	{
		memcpy(packet,Real[match].mac,6);
		memcpy(packet+6,eth_int[Real[match].interfaceposn].mac,6);
	}	
	int rr;
	rr=pcap_inject(desc[Real[match].interfaceposn],packet,len);
	if(rr<0)
	{
		printf("\nFailed\n");
		exit(0);
	}
	return;
}	

