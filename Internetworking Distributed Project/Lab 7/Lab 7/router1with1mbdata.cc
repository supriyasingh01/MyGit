//Server Code for Router_1
/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <pcap.h> /* if this gives you an error try pcap/pcap.h */
#include <errno.h>
#include <netinet/ether.h>
#include <pthread.h>
#include <fstream>
#include <iostream>
#include <sys/wait.h>
#include <map>
#include <sstream>
#include <netinet/ip_icmp.h> //icmp header format got from here
using namespace std;


//MACROS
#define PKTSIZE 100
#define SNAP_LEN 1518 //default snap length (maximum bytes per packet to capture
#define SIZE_ETHERNET 14 //ethernet headers are always exactly 14 bytes [1] 

typedef unsigned short u16;
typedef unsigned long u32;

void get_mac_addr(char *s_ip, char *s_mac, char *d_ip, char *iface, unsigned char *d_mac);
void *startRouting(void *device);

/* Ethernet header */
struct sniff_ethernet {
        u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
        u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
        u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
        u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
        u_char  ip_tos;                 /* type of service */
        u_short ip_len;                 /* total length */
        u_short ip_id;                  /* identification */
        u_short ip_off;                 /* fragment offset field */
        #define IP_RF 0x8000            /* reserved fragment flag */
        #define IP_DF 0x4000            /* dont fragment flag */
        #define IP_MF 0x2000            /* more fragments flag */
        #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
        u_char  ip_ttl;                 /* time to live */
        u_char  ip_p;                   /* protocol */
        u_short ip_sum;                 /* checksum */
        struct  in_addr ip_src,ip_dst;  /* source and dest address */
};
#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

struct sniff_tcp {
        u_short th_sport;               /* source port */
        u_short th_dport;               /* destination port */
        tcp_seq th_seq;                 /* sequence number */
        tcp_seq th_ack;                 /* acknowledgement number */
        u_char  th_offx2;               /* data offset, rsvd */
        
		#define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
        u_char  th_flags;
        #define TH_FIN  0x01
        #define TH_SYN  0x02
        #define TH_RST  0x04
        #define TH_PUSH 0x08
        #define TH_ACK  0x10
        #define TH_URG  0x20
        #define TH_ECE  0x40
        #define TH_CWR  0x80
        #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
        u_short th_win;                 /* window */
        u_short th_sum;                 /* checksum */
        u_short th_urp;                 /* urgent pointer */
};

//Packet
typedef struct PacketData 
{
	long seqNo;									//sequence number of the packet of 8 bytes each
	size_t dataLength;							//length of the packet of 8 bytes each
	unsigned char data[PKTSIZE-sizeof(long)-sizeof(size_t)];	//data from the file of 1 byte each
}Packet;

struct adapter_info
{
	string ip;
	string mac;
	string iface;
	pcap_t *handle;
};
map<string,struct adapter_info*> ifconfig;

struct route_record
{
	int networkaddr;
	int mask;
	int gateway_ip;
	unsigned char gateway_mac[6];
	string interface;
};

map<unsigned int,int> masks;

class routing
{
	public:
	map<string,struct route_record*> table;
	int no_of_entries;

	routing()
	{
		no_of_entries=0;
	}		

	void add_entry(string nwaddr,string msk,string gw_ip,string iface)
	{
		struct route_record* temp = new route_record;
		std::stringstream temp_map_ins;
		char buf[1024];
		temp->networkaddr = ntohl(inet_addr(nwaddr.c_str()));
		temp->mask = ntohl(inet_addr(msk.c_str()));
		temp->gateway_ip = ntohl(inet_addr(gw_ip.c_str()));
		temp->interface = iface;
		sprintf(buf, "%u_%u",temp->networkaddr,temp->mask );
		map<string,struct adapter_info*>::iterator it;

		it = ifconfig.find(iface);
		if(it != ifconfig.end())
		{
			struct adapter_info* temp_adpinfo = (*it).second;
			get_mac_addr((char*)temp_adpinfo->ip.c_str(),(char*)temp_adpinfo->mac.c_str(),(char*)gw_ip.c_str(),(char*)iface.c_str(),temp->gateway_mac);
		}
		else
		{
			printf("no such interface\n");
		}
		
		table.insert(pair<string,struct route_record*>(buf,temp));
		cout<<"Network Address: "<<temp->networkaddr<<" Mask: "<<temp->mask<<" Next Hop IP: "<<temp->gateway_ip<<" Interface: "<<temp->interface<<" connecting to router2"<<endl;
		printf("MAC: %x:%x:%x:%x:%x:%x\n",temp->gateway_mac[0],temp->gateway_mac[1],temp->gateway_mac[2],temp->gateway_mac[3],temp->gateway_mac[4],temp->gateway_mac[5]);
		//cout<<"Table Size is "<<table.size()<<endl;		
	}
};

routing rtr;

string get_local_mac(char *iface)
{
	int s;
	struct ifreq buffer;
	string ret_mac;
	s = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&buffer, 0x00, sizeof(buffer));
	strcpy(buffer.ifr_name, iface);
	ioctl(s, SIOCGIFHWADDR, &buffer);
	close(s);
	ret_mac = ether_ntoa((ether_addr *)buffer.ifr_hwaddr.sa_data);
	return ret_mac;
}

void calc_ifcofig()
{
	char *dev, errbuf[PCAP_ERRBUF_SIZE];
	//dev = pcap_lookupdev(errbuf);
	dev = "eth0";
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
			exit(1);
	}
	
	struct adapter_info *temp = new adapter_info;
	temp->ip.assign("10.1.2.3"); // ip address of router having eth0 that is connected to Node0
	temp->mac = get_local_mac(dev);
	temp->iface.assign(dev);
	cout<<endl<<" ip address is "<<temp->ip<<" for router1 with interface "<<temp->iface<<endl;
	cout<<"MAC "<<get_local_mac(dev)<<endl<<endl;
	ifconfig.insert(pair<string,struct adapter_info*>(temp->iface,temp));
	temp->handle = pcap_open_live((char *)dev, SNAP_LEN, 1, 1000, errbuf);
	
	dev = "eth1";
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
			exit(1);
	}
	
	struct adapter_info *tempTwo = new adapter_info;
	tempTwo->ip.assign("10.1.3.2"); // ip address of router having eth1 that is connected to Router2
	tempTwo->mac = get_local_mac(dev);
	tempTwo->iface.assign(dev);
	cout<<" ip address is "<<tempTwo->ip<<" for router1 with interface "<<tempTwo->iface<<endl;
	cout<<"MAC "<<get_local_mac(dev)<<endl;
	ifconfig.insert(pair<string,struct adapter_info*>(tempTwo->iface,tempTwo));
	tempTwo->handle = pcap_open_live((char *)dev, SNAP_LEN, 1, 1000, errbuf);
	
}

void read_table()
{
	ifstream file;

	file.open("static_routes");

	if(!file.good())
	{
		printf("Error opening static_routes file\n");
		exit(0);
	}
	printf("\n\nRouting table: \n\n");
	while(file.good())
	{
		char line[1024]={'\0'};
		file.getline(line,1024);
		
		string str_line;
		str_line.assign(line,file.gcount());
		int loc=-1;
		int count=1;
		string ip,gw_ip,msk,iface;
		bool found = false;

		while((loc=str_line.find(","))!=-1)
		{
			found = true;
			string temp;
			temp = str_line.substr(0,loc);
			
			switch(count)
			{
				case 1:
					ip = temp;
				break;
				case 2:
					msk = temp;
				break;
				case 3:
					gw_ip = temp;
				break;
				default:
				{
					printf("Error in file!\n");
					exit(1);
				}
				break;
			}
			str_line = str_line.substr(loc+1,str_line.length());
			count++;
		}
		iface = str_line.substr(0,4);
		if(found)
		{
			masks.insert(pair<unsigned int,int>(ntohl(inet_addr(msk.c_str())),0));
			//mainMsk=ntohl(inet_addr(msk.c_str()));
			rtr.add_entry(ip,msk,gw_ip,iface);
		}		
	}
}
void error(const char *msg) /* This function is called when a system call fails. */
							/*It displays a message about the error on stderr and then aborts the program.*/
{
    perror(msg);
    exit(1);
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{

	static int count = 1;                   /* packet counter */
	
	/* declare pointers to packet headers */
	struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	struct sniff_ip *ip;              		/* The IP header */
	struct icmphdr *icmp;
    u_char *pkt_data;
	int size_ip;
	int size_payload;
	struct sniff_tcp* tcp;
	int size_tcp;
	u_char *payload;                    /* Packet payload */
	
	printf("\nPacket number %d:\n", count);
	count++;
	
	/* define ethernet header */
	ethernet = (struct sniff_ethernet*)(packet);
	
	/* define/compute ip header offset */
	ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
	size_ip = IP_HL(ip)*4;
	if (size_ip < 20) {
		printf("   * Invalid IP header length: %u bytes\n", size_ip);
		return;
	}

	/* print source and destination IP addresses */
	printf("       From: %s\n", inet_ntoa(ip->ip_src));
	printf("         To: %s\n", inet_ntoa(ip->ip_dst));
	
	/* determine protocol */	
	switch(ip->ip_p) {
		case IPPROTO_TCP:
		{
			printf("   Protocol: TCP\n");
			tcp = (struct sniff_tcp*)(packet + SIZE_ETHERNET + size_ip);
			size_tcp = TH_OFF(tcp)*4;
			if (size_tcp < 20) 
			{
				printf("   * Invalid TCP header length: %u bytes\n", size_tcp);
				return;
			}
			
			//...............................TCP Connection.........................................................//
			
			int sockfd, newsockfd, portno;/* portno-- stores the port number on which the server accepts connections */
     		socklen_t clilen;/*clilen stores the size of the address of the client*/
     		pid_t pid;
     		struct sockaddr_in serv_addr, cli_addr;/*containing an internet address. */
     		int n;/*it contains the number of characters read or written*/
     		if (2 < 2) 
     		{
         		fprintf(stderr,"ERROR, no port provided\n");
         		exit(1);
     		}  
     		sockfd = socket(AF_INET, SOCK_STREAM, 0);/* AF_INET is internet domain*/ 
     		if (sockfd < 0) error("ERROR opening socket");   
     		bzero((char *) &serv_addr, sizeof(serv_addr));
     		portno = atoi("3456");//The port number on which the server will listen is HARDCODED
     		serv_addr.sin_family = AF_INET;
     		serv_addr.sin_addr.s_addr = INADDR_ANY;
     		serv_addr.sin_port = htons(portno);
    		if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");
     		listen(sockfd,5);/*allows the process to listen on the socket for connections*/
     		clilen = sizeof(cli_addr);
    		 
    		while(1)
     		{
     			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);
     			if (newsockfd < 0) error("ERROR on accept");   
        		pid = fork();
   				if (pid < 0)error("ERROR on fork");
   				if (pid == 0)
   				{
     				close(sockfd);// Parent socket
     				long packetCount = 0;// For now its hard coeded  to 729444 from the value of client side
					int recvd=0;
					int dataSent=0;
					FILE * pFile;
					//pFile = fopen ("test","w+");
					int m;
	
     				while (packetCount<=10486) 
					{
						Packet *dataReceived = NULL;
						dataReceived = (Packet*)malloc(sizeof(Packet));
						memset(dataReceived->data,'\0',sizeof(dataReceived->data));	
						recvd = read(newsockfd,dataReceived,sizeof(Packet));
						printf("%d bytes recievd by the server in for %ld packet \n", recvd, dataReceived->seqNo);
						//m=fwrite(dataReceived->data,1,dataReceived->dataLength,pFile);  
						if (recvd < 0) 
						{
							error("read");
							exit(0);
						}
						packetCount++;
					}
					packetCount--;// Decrement bcoz while loop ends only when packetCount exceeds actual count 
    				printf("The no. of pkts =%ld\n",packetCount);
    				
     				/* define/compute tcp header offset */
     				printf("   Src port: %d\n", ntohs(tcp->th_sport));
					printf("   Dst port: %d\n", ntohs(tcp->th_dport));
     				exit(0);
				} 
				else close(newsockfd);
     		}
			break;
			//return;		
		}					
		case IPPROTO_UDP:
			printf("   Protocol: UDP\n");
			break;
			//return;
		case IPPROTO_ICMP:
			printf("   Protocol: ICMP\n");
			break;
			//return;
		case IPPROTO_IP:
			printf("   Protocol: IP\n");
			break;
			//return;
		default:
			printf("   Protocol: unknown\n");
			break;
			//return;
	}
}	
int main(int argc, char *argv[])
{
	calc_ifcofig();	
	read_table();
	
	cout<<"Creating Threads"<<endl;
	pthread_t ThreadRouting;
	int returnValThread = 0; 
	returnValThread = pthread_create(&ThreadRouting, NULL, startRouting, (void *)"eth0");
	pthread_join(ThreadRouting,NULL);
	
  return 0; 
}

void *startRouting(void *dev)
{	
	char *device=(char *)dev;//"eth0";			//change required - take device from ifconfig
	printf("\n\nSniffing on device %s",device);
	
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle=NULL;				/* packet capture handle */
	int ret;

	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */
	
	/* get network number and mask associated with capture device */
	
	if (pcap_lookupnet((char *)device, &net, &mask, errbuf) == -1) 
	{
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n",(char *)device, errbuf);
		net = 0;
		mask = 0;
	}
	
	map<string,struct adapter_info*>::iterator it_listen_adapter;
	it_listen_adapter = ifconfig.find((char*)device);
	if(it_listen_adapter != ifconfig.end())
	{
		struct adapter_info* temp_listen_adpinfo = (*it_listen_adapter).second;
		handle = temp_listen_adpinfo->handle;
	}
	else
		printf("error finding sending device info in router thread\n");
	
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", device, errbuf);
		exit(EXIT_FAILURE);
	}
	/* now we can set our callback function */
	pcap_loop(handle, -1, got_packet, NULL);
	/* cleanup */
	//pcap_freecode(&fp);
	pcap_close(handle);
	printf("\nCapture complete.\n");
}	

