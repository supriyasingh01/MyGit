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

int main(int argc, char **argv)
{
	int j=0;
	int s;
	struct ifreq ifr;
	FILE *fp;
	struct stat fileStatus;
	char segment[50];
	
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

	unsigned char src_mac[6]  = {0x27, 0x01, 0x02, 0xFA, 0x70, 0xAA};
	unsigned char dest_mac[6] = {0x23, 0x04, 0x75, 0xC8, 0x28, 0xE5};

	s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (s == -1) 
	{
		perror(argv[0]);
	}
	
	memset(&ifr,0, sizeof(struct ifreq));
    strncpy((char *)ifr.ifr_name,"eth2",IFNAMSIZ);
	
    if(ioctl(s, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl SIOCGIFINDEX");
        exit(1);
    }

    //printf("index %d\n",ifr.ifr_ifindex);
	
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
	socket_address.sll_addr[1]  = 0x04;		
	socket_address.sll_addr[2]  = 0x75;
	socket_address.sll_addr[3]  = 0xC8;
	socket_address.sll_addr[4]  = 0x28;
	socket_address.sll_addr[5]  = 0xE5;
	/*MAC - end*/
	socket_address.sll_addr[6]  = 0x00;/*not used*/
	socket_address.sll_addr[7]  = 0x00;/*not used*/
	

	/*set the frame header*/
	memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
	memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
	eh->h_proto = 0x00;

	stat(argv[1], &fileStatus);
	long fileSize=fileStatus.st_size;	
	fp = fopen(argv[1],"r");
	if (fp==NULL)
	{
		printf("Error opening file!\n");
		fclose (fp);
	}	

	int result = 0;	
	char initial[100];

	memset(data,'\0',1500);
	int totalPackets = (fileSize/50.0)+1;
	printf("Size: %ld\n",fileSize);
	printf("Number of packets: %d\n",totalPackets);
	sprintf(initial,"%d",totalPackets);
	//printf("Initial: %s, %d\n",initial,strlen(initial));	
	for (j = 0; j < strlen(initial); j++) 
	{
		data[j] = initial[j];
	}	
	//printf("Initial: %s\n",data);	
	/*send the packet*/
	res = sendto(s, buffer, ETH_FRAME_LEN, 0,
			   (struct sockaddr*)&socket_address, sizeof(socket_address));
	if (res == -1) 
	{
		printf("Initial packet not sent!\n");
		exit(0);
	}

	int counter=0;
	
	memset(segment,'\0',50);
	while(!feof(fp))
	{
		memset(segment,'\0',50);
		memset(data,'\0',1500);
		result=fread (segment,1,50,fp);
		if (result <= 0)
		{
			perror ("Reading error : "); 
			exit (-1);
		}
		//printf("Packet: %s\n",segment);
		/*fill the frame with some data*/
		for (j = 0; j < result; j++) 
		{
			data[j] = segment[j];
		}
		//printf("Packet: %s\n",data);
		/*send the packet*/
		res = sendto(s, buffer, ETH_FRAME_LEN, 0,
			   (struct sockaddr*)&socket_address, sizeof(socket_address));
		if (res == -1) 
		{
			perror(argv[0]);
		}
		//if(counter % 10 ==0)
		//{
			//printf("Enter here... \n");
			usleep(1);
		//}
		counter++;
	}
	fclose(fp);
	close(s);	
	return 0;
}