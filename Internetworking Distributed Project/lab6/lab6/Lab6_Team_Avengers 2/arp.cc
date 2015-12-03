#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h> /* write and close functions */
#include<sys/socket.h> /* contains socket(), bind(), includes SOCK_RAW macro from bits/socket.h */
#include<features.h>
#include<linux/if_packet.h> /* contains sockaddr_ll */
#include<linux/if_ether.h> /* contain 802.3 protocol numbers and constants, contains definition of ethernet frame header */
#include<errno.h>
#include<sys/ioctl.h> /* contains ifreq, includes SIOCGIFINDEX */
#include<net/if.h>
#include<netinet/ether.h>
#include<netinet/in.h>
#include<netinet/if_ether.h> /* contains ether_arp struct */
#include<net/if_arp.h> /* contains arphdr struct and ARP protocol HARDWARE identifiers.*/
#include<arpa/inet.h> /* has inet_ptoa() */

#include <iostream>
using namespace std;

//http://www.tcpipguide.com/free/t_ARPMessageFormat.htm
//Conversion functions http://www.informit.com/articles/article.aspx?p=169505&seqNum=6

void get_mac_addr(char *s_ip, char *s_mac, char *d_ip, char *iface, unsigned char *d_mac)
{
	//cout<<"s_ip "<<s_ip<<endl;
	//cout<<"s_mac "<<s_mac<<endl;
	//cout<<"d_ip "<<d_ip<<endl;
	//cout<<"iface "<<iface<<endl;

	// set up the socket, which is simply an integer
	int rawsock;
	//format socket(int socket_family, int socket_type, int protocol);
	if((rawsock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		perror("Error creating raw socket");
		exit(1);
	}
	// end of setting up socket


	// bind interface eth0 to the socket
	struct sockaddr_ll sll;
	struct ifreq ifr;

	bzero(&sll, sizeof(sll));
	bzero(&ifr, sizeof(ifr));

	/* First Get the Interface Index  */
	strncpy((char *)ifr.ifr_name, iface, 4);
	if((ioctl(rawsock, SIOCGIFINDEX, &ifr)) == -1)
	{
		perror("Error getting Interface index!\n");
		exit(1);
	}

	/* Bind our raw socket to this interface */
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(ETH_P_ALL);


	if((bind(rawsock, (struct sockaddr *)&sll, sizeof(sll)))== -1)
	{
		perror("Error binding raw socket to interface\n");
		exit(1);
	}
	//end of binding interface to socket
	
	//create parts of the packet
	char destination[] = "FF:FF:FF:FF:FF:FF";

	char source[18];
	char src_ip[18];
	char dst_ip[18];

	sprintf(source,"%s",s_mac);
	sprintf(src_ip,"%s",s_ip);
	sprintf(dst_ip,"%s",d_ip);

	struct ethhdr ethernet_header;
	bzero(&ethernet_header, sizeof(ethernet_header));
	memcpy(ethernet_header.h_dest, (void *)(ether_aton(destination)), 6);
	memcpy(ethernet_header.h_source, (void *)(ether_aton(source)), 6);
	ethernet_header.h_proto = htons(ETH_P_ARP);

	struct ether_arp arp_packet;
	bzero(&arp_packet, sizeof(arp_packet));
	arp_packet.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	arp_packet.ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	arp_packet.ea_hdr.ar_hln = ETHER_ADDR_LEN;
	arp_packet.ea_hdr.ar_pln = 0x4;
	arp_packet.ea_hdr.ar_op = htons(ARPOP_REQUEST);


	memcpy(arp_packet.arp_sha, (void *)(ether_aton(source)), 6);
	if(inet_pton(AF_INET, src_ip, (void *)arp_packet.arp_spa) == 0 )
	{
		printf("Invalid representation format for the source address\n");
	}
	memcpy(arp_packet.arp_tha, (void *)(ether_aton(destination)), 6);
	if(inet_pton(AF_INET, dst_ip, (void *)arp_packet.arp_tpa) == 0 )
	{
		printf("Invalid representation format for the source address \n");
	}

	//pack and pad the packet
	unsigned char packet[sizeof(ethernet_header) + sizeof(arp_packet) + 18]; //has to add up to 60
	unsigned char reply_packet[sizeof(ethernet_header) + sizeof(arp_packet) + 18];
	bzero(packet, sizeof(packet));
	memcpy(packet, &ethernet_header, sizeof(ethernet_header));
	memcpy(packet + sizeof(ethernet_header), &arp_packet, sizeof(arp_packet));

	//end of creating a packet

	//write packet to socket
	if(write(rawsock, (unsigned char *)(&packet), sizeof(packet)) == -1)
	{
		perror("Error writing bytes to the socket! \n");
		exit(1);
	}

	memset(reply_packet,0,sizeof(reply_packet));

	if(read(rawsock,reply_packet,sizeof(reply_packet))==-1)
	{
		perror("Error reading bytes from the socket! \n");
		exit(1);
	}

	//printf("reply:\n%s\n",reply_packet);
	
	memcpy(d_mac,&reply_packet[22],6);

	//close the socket
	if(close(rawsock) == -1)
	{
		perror("Error closing the socket! \n");
		exit(1);
	}

} //end of main()


