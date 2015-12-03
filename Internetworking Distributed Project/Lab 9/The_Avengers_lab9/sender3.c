#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv)
{
	int j=0;
	int s;
	struct ifreq ifr;
	FILE *fp;
	struct stat fileStatus;
	char segment[50];
	srand ( time(NULL) );
	int colorCode;
	struct sockaddr_ll socket_address;
	bzero(&socket_address, sizeof(socket_address));
	bzero(&ifr, sizeof(ifr));
		
	/*buffer for ethernet frame*/
	void* buffer = (void*)malloc(ETH_FRAME_LEN);

	/*pointer to ethenet header*/
	unsigned char* etherhead = (unsigned char*)buffer;

	unsigned char* data = (unsigned char*)buffer + 14;

	struct ethhdr *eh = (struct ethhdr *)etherhead;

	int res = 0;

	unsigned char src_mac[6]  = {0x00, 0x04, 0x23, 0xC5, 0xD4, 0x72};
	unsigned char dest_mac[6] = {0x00, 0x4E, 0x46, 0x32, 0x43, 0x03};

	s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (s == -1) 
	{
		printf("Error creating raw socket!\n");
		exit(0);
	}
	
	memset(&ifr,0, sizeof(struct ifreq));
    strncpy((char *)ifr.ifr_name,"eth0",IFNAMSIZ);
	
    if(ioctl(s, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl SIOCGIFINDEX");
        exit(1);
    }

    printf("index %d\n",ifr.ifr_ifindex);
	
	/*RAW communication*/
	socket_address.sll_family   = AF_PACKET;	
	socket_address.sll_ifindex = ifr.ifr_ifindex;
	/*we don't use a protocoll above ethernet layer
	  ->just use anything here*/
	socket_address.sll_protocol = htons(ETH_P_ALL);	

	/*ARP hardware identifier is ethernet*/
	socket_address.sll_hatype   = ARPHRD_ETHER;
		
	/*target is another host*/
	socket_address.sll_pkttype  = PACKET_OTHERHOST;

	/*address length*/
	socket_address.sll_halen    = ETH_ALEN;	

	/*MAC - begin*/
	socket_address.sll_addr[0]  = 0x00;		
	socket_address.sll_addr[1]  = 0x4E;		
	socket_address.sll_addr[2]  = 0x46;
	socket_address.sll_addr[3]  = 0x32;
	socket_address.sll_addr[4]  = 0x43;
	socket_address.sll_addr[5]  = 0x03;
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/
	

	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = 0x00;

	int counter=0,result;
	
	memset(segment,'\0',50);
	while(1)
	{
		memset(segment,'\0',50);
		memset(data,'\0',1500);
		/* generate color code: */
		colorCode = rand() % 4 + 1;
		printf("Color: %d\n",colorCode);
		switch(colorCode)
		{
			case 1:memcpy(segment,"RED",3); result=3;break;
			case 2:memcpy(segment,"BLUE",4); result=4;break;
			case 3:memcpy(segment,"GREEN",5); result=5;break;
			case 4:memcpy(segment,"YELLOW",6); result=6;break;
		}
		/*fill the frame with some data*/
		for (j = 0; j < result; j++) 
		{
			data[j] = segment[j];
		}
		printf("Packet: %s\n",data);
		/*send the packet*/
		res = sendto(s, buffer, ETH_FRAME_LEN, 0,
			   (struct sockaddr*)&socket_address, sizeof(socket_address));
		if (res == -1) 
		{
			printf("Error sending data!\n");
			exit(0);
		}
		if(counter % 1000 ==0)
		{
			printf("Enter here... \n");
			usleep(10);
		}
		counter++;
	}
	close(s);	
	return 0;
}
