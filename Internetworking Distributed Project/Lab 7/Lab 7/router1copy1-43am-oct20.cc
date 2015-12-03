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

void get_mac_addr(char *s_ip, char *s_mac, char *d_ip, char *iface, unsigned char *d_mac);

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

int main(int argc, char *argv[])
{
	calc_ifcofig();	
	read_table();
     return 0; 
}
