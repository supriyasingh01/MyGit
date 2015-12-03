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
#include <map>
#include <sstream>
#include <netinet/ip_icmp.h> //icmp header format got from here
using namespace std;

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

typedef unsigned short u16;
typedef unsigned long u32;

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
unsigned int mainMsk;

void get_mac_addr(char *s_ip, char *s_mac, char *d_ip, char *iface, unsigned char *d_mac);
void *startRouting(void *device);

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
		cout<<"Network Address: "<<temp->networkaddr<<" Mask: "<<temp->mask<<" Next Hop IP: "<<temp->gateway_ip<<" Interface: "<<temp->interface<<endl;
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
	//int  i=0;
	pcap_if_t *alldevs,*d;
	char errbuf[PCAP_ERRBUF_SIZE];

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		   fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
		   exit(1);
	}

	if(alldevs==NULL)
	{
		   printf("Error\n");
	}

	for(d=alldevs; d; d=d->next)
	{
		if(d->addresses!=NULL)
		{
			pcap_addr *next = d->addresses;
			int iface_count=0;
			do
			{	   
				sockaddr * p = next->addr;
				if(p!=NULL)
				{
					if(p->sa_family==AF_INET)
					{
						char s[1024]={'\0'};
						inet_ntop(p->sa_family, &(p->sa_data[2]),s,1024);

						struct adapter_info *temp = new adapter_info;
						temp->ip.assign(s,strlen(s));
						temp->mac = get_local_mac(d->name);
						temp->iface.assign(d->name);
						//printf("%s ",d->name);
						//printf("IP: %s",s);
						//cout<<"MAC "<<get_local_mac(d->name)<<endl;
						cout<<"IP "<<temp->ip<<" MAC "<<temp->mac<<" iface "<<temp->iface<<endl;
						ifconfig.insert(pair<string,struct adapter_info*>(temp->iface,temp));
						temp->handle = pcap_open_live((char *)d->name, SNAP_LEN, 1, 1000, errbuf);
					}
				}
				next = next->next;
				iface_count++;
			}
			while (next!=NULL);
		}
	}
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
				{
					ip = temp;
				}
				break;
				case 2:
				{
					msk = temp;
				}
				break;
				case 3:
				{
					gw_ip = temp;
				}
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
			mainMsk=ntohl(inet_addr(msk.c_str()));
			rtr.add_entry(ip,msk,gw_ip,iface);
		}		
	}
}

//http://www.unix.com/programming/117551-calculate-ip-header-checksum-manually.html
unsigned short checksum(unsigned short *ptr, int length)
{
	register int sum = 0;
	u_short answer = 0;
	register u_short *w = ptr;
	register int nleft = length;
	while(nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	answer = ~sum;
	return(answer);
}

//http://www.netfor2.com/ipsum.htm
u16 ip_sum_calc(u16 len_ip_header, u16 buff[])
{
u16 word16;
u32 sum=0;
u16 i;
    
	// make 16 bit words out of every two adjacent 8 bit words in the packet
	// and add them up
	for (i=0;i<len_ip_header;i=i+2){
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (u32) word16;	
	}
	
	// take only 16 bits out of the 32 bit sum and add up the carries
	while (sum>>16)
	  sum = (sum & 0xFFFF)+(sum >> 16);

	// one's complement the result
	sum = ~sum;
	
return ((u16) sum);
}

void
got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{

	static int count = 1;                   /* packet counter */
	
	/* declare pointers to packet headers */
	struct sniff_ethernet *ethernet;  /* The ethernet header [1] */
	struct sniff_ip *ip;              		/* The IP header */
	struct icmphdr *icmp;
    u_char *pkt_data;
	int size_ip;
	int size_payload;
	
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
			printf("   Protocol: TCP\n");
			break;
			//return;							//change Prachi
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
	
	unsigned int nw_addr_look_up;
	unsigned int ip_addr = ntohl(inet_addr(inet_ntoa(ip->ip_dst)));
	//find network address by anding mask with ip address
	nw_addr_look_up=ip_addr & mainMsk;						
	//printf("here i am..\n");
	printf("IP Address: %u\nMask: %u\nNetwork Address:%u\n",ip_addr,mainMsk,nw_addr_look_up);
	
	//now lookup this network address in the routing table
	char buff[1024];
	sprintf(buff, "%u_%u",nw_addr_look_up,mainMsk);
	map<string,struct route_record*>::iterator it;
	it = rtr.table.find(buff);
	if(it != rtr.table.end())
	{
		printf("Found in table!\n");	
		
		u_char local_mac[6];
		pcap_t *reply_handle=NULL;
		u_char reply[SNAP_LEN];		
		memset(reply,0,SNAP_LEN);
		memcpy(reply,packet,header->len);			
		struct route_record *temp_rec = (*it).second;
		
		//find source MAC (local) and the handle for pcap inject
		map<string,struct adapter_info*>::iterator it_send_adapter;
		it_send_adapter = ifconfig.find((char*)temp_rec->interface.c_str());
		if(it_send_adapter != ifconfig.end())
		{
			struct adapter_info* temp_send_adpinfo = (*it_send_adapter).second;
			memcpy(local_mac,(void *)ether_aton(temp_send_adpinfo->mac.c_str()),6);
			reply_handle = temp_send_adpinfo->handle;		
		}
		
		//create the packet
		
		if((ip->ip_ttl=(u_char)((int)(ip->ip_ttl)-1))>0)
			printf("TTL: %d\n",ip->ip_ttl);
		else
			{
			//Declarations
			u_char local_mac[6];
			u_char reply[SNAP_LEN];
			pcap_t *reply_handle=NULL;
			struct route_record *temp_rec = (*it).second;
		
			printf("Not found!\nSending ICMP dest unreachable message\n");
			ethernet = (struct sniff_ethernet *)packet;
			ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
			size_ip = IP_HL(ip)*4;
		
			memset(reply,0,SNAP_LEN);
			memcpy(reply,packet,header->len);
		
			//find source MAC (local) and the handle for pcap inject
			map<string,struct adapter_info*>::iterator it_send_adapter;
			it_send_adapter = ifconfig.find((char*)temp_rec->interface.c_str());
			if(it_send_adapter != ifconfig.end())
			{
				struct adapter_info* temp_send_adpinfo = (*it_send_adapter).second;
				memcpy(local_mac,(void *)ether_aton(temp_send_adpinfo->mac.c_str()),6);
				reply_handle = temp_send_adpinfo->handle;		
			}
		
			icmp = (struct icmphdr *)(ip + size_ip);
			icmp->type = (u_char)(int)11;
			icmp->code = (u_char)(int)0;
			icmp->checksum = htons(~(11 << 8));
		
			u_char *data = (u_char*) (packet + SIZE_ETHERNET + size_ip + 8);
			memcpy(data,ip,((IP_HL(ip)*4)+8));
		
			ip->ip_ttl=(u_char)(int) 64;
			ip->ip_len = (u_char)(int) 56;
			ip->ip_dst = ip->ip_src;
			inet_aton("10.99.0.3", (in_addr*)&ip->ip_src);
			//ip->ip_src = inet_addr("10.99.0.3");//local ip address
		
			(ip->ip_sum) = checksum((unsigned short *)ip,20);
		
			struct sniff_ethernet *ethernet_send;  /* The ethernet header [2] */
		
			/* define ethernet header */
			ethernet_send = (struct sniff_ethernet*)(reply);
		
			//Set the source MAC address to the MAC address of the ethernet adapter on your router that will be sending the packet
			//Set the destination MAC address to the MAC address of the next-hop.
			memcpy(ethernet_send->ether_dhost,temp_rec->gateway_mac,6);
			memcpy(ethernet_send->ether_shost,local_mac,6);
		
			pcap_sendpacket(reply_handle, (const u_char*)reply, header->len);
			printf("Sent ICMP unreachable\n");
		}
	
		(ip->ip_sum)= checksum((unsigned short *)ip,20);
		
		struct sniff_ethernet *ethernet_send;  /* The ethernet header [2] */
		/* define ethernet header */
		ethernet_send = (struct sniff_ethernet*)(reply);
		
		//Set the source MAC address to the MAC address of the ethernet adapter on your router that will be sending the packet
		//Set the destination MAC address to the MAC address of the next-hop.
		memcpy(ethernet_send->ether_dhost,temp_rec->gateway_mac,6);
		memcpy(ethernet_send->ether_shost,local_mac,6);
		
		//forward the packet!
		pcap_sendpacket(reply_handle, (const u_char*)reply, header->len);
		printf("Sent!\n");
	}
	else
	{
		//Declarations
		u_char local_mac[6];
		u_char reply[SNAP_LEN];
		pcap_t *reply_handle=NULL;
		struct route_record *temp_rec = (*it).second;
		
		printf("Not found!\nSending ICMP dest unreachable message\n");
		ethernet = (struct sniff_ethernet *)packet;
		ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
		size_ip = IP_HL(ip)*4;
		
		memset(reply,0,SNAP_LEN);
		memcpy(reply,packet,header->len);
		
		//find source MAC (local) and the handle for pcap inject
		map<string,struct adapter_info*>::iterator it_send_adapter;
		it_send_adapter = ifconfig.find((char*)temp_rec->interface.c_str());
		if(it_send_adapter != ifconfig.end())
		{
			struct adapter_info* temp_send_adpinfo = (*it_send_adapter).second;
			memcpy(local_mac,(void *)ether_aton(temp_send_adpinfo->mac.c_str()),6);
			reply_handle = temp_send_adpinfo->handle;		
		}
		
		icmp = (struct icmphdr *)(ip + size_ip);
		icmp->type = (u_char)(int)ICMP_UNREACH;
		icmp->code = (u_char)(int)ICMP_UNREACH_HOST;
		icmp->checksum = htons(~(ICMP_UNREACH << 8));
		
		u_char *data = (u_char*) (packet + SIZE_ETHERNET + size_ip + 8);
		memcpy(data,ip,((IP_HL(ip)*4)+8));
		
		ip->ip_ttl=(u_char)(int) 64;
		ip->ip_len = (u_char)(int) 56;
		ip->ip_dst = ip->ip_src;
		inet_aton("10.99.0.3", (in_addr*)&ip->ip_src);
		//ip->ip_src = inet_addr("10.99.0.3");//local ip address
		
		(ip->ip_sum) = checksum((unsigned short *)ip,20);
		
		struct sniff_ethernet *ethernet_send;  /* The ethernet header [2] */
		
		/* define ethernet header */
		ethernet_send = (struct sniff_ethernet*)(reply);
		
		//Set the source MAC address to the MAC address of the ethernet adapter on your router that will be sending the packet
		//Set the destination MAC address to the MAC address of the next-hop.
		memcpy(ethernet_send->ether_dhost,temp_rec->gateway_mac,6);
		memcpy(ethernet_send->ether_shost,local_mac,6);
		
		pcap_sendpacket(reply_handle, (const u_char*)reply, header->len);
		printf("Sent ICMP unreachable\n");
	}
}

	
int main(int argc, char *argv[]) 
{
	calc_ifcofig();	
	read_table();
	cout<<"Creating Threads"<<endl;

	pthread_t Thread1,Thread2;
	int tret = 0; 
	tret = pthread_create(&Thread1, NULL, startRouting, (void *)"eth1");
	tret = pthread_create(&Thread2, NULL, startRouting, (void *)"eth0");
	pthread_join(Thread1,NULL);
	pthread_join(Thread2,NULL);
	return 0;
}	
	
	
void *startRouting(void *dev)
{	
	//------

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
	{
		printf("error finding sending device info in router thread\n");
	}
	
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", device, errbuf);
		exit(EXIT_FAILURE);
	}
	
	/* now we can set our callback function */
	pcap_loop(handle, 100, got_packet, NULL);

	/* cleanup */
	//pcap_freecode(&fp);
	pcap_close(handle);

	printf("\nCapture complete.\n");
	

	
	
	//---
}	
