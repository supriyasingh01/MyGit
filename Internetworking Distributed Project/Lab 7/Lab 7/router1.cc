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

string get_local_mac(char *iface)
{
	printf("inside get_local_mac\n");
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
	printf("inside cal_ifcofig\n");
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
		printf("inside cal_ifcofig loop\n");   
		if(d->addresses!=NULL)
		{
			printf("inside cal_ifcofig loop if\n");   
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

void error(const char *msg) /* This function is called when a system call fails. */
							/*It displays a message about the error on stderr and then aborts the program.*/
{
    perror(msg);
    exit(1);
}
void dostuff(int newsockfd)
{
	//Calling function to get local adresses
    calc_ifcofig();	
    
	long packetCount = 0;// For now its hard coeded  to 729444 from the value of client side
	int recvd=0;
	int dataSent=0;
	
	while (packetCount<=10486) 
	{
		Packet *dataReceived = NULL;
		dataReceived = (Packet*)malloc(sizeof(Packet));
		memset(dataReceived->data,'\0',sizeof(dataReceived->data));	
		recvd = read(newsockfd,dataReceived,sizeof(Packet));
		printf("%d bytes recievd by the server in for %ld packet \n", recvd, dataReceived->seqNo);
		if (recvd < 0) error("read");
		packetCount++;
	}
	packetCount--;// Decrement bcoz while loop ends only when packetCount exceeds actual count 
    
    //Sending message to client
    printf("The no. of pkts =%ld\n",packetCount);
    char NumOfPacketsRecvd[32];
    for(int i=0;i<32;i++)
    	NumOfPacketsRecvd[i] = '\0';
    
    sprintf(NumOfPacketsRecvd,"%ld",packetCount);
    dataSent = write(newsockfd,NumOfPacketsRecvd,32);
    if (dataSent < 0) error("ERROR writing to socket");	
    
}
void *SigCatcher(int signo)/*Reference : http://souptonuts.sourceforge.net/code/server2way_waitpid.c.html*/
{
  //wait3(NULL,WNOHANG,NULL);
   pid_t pid;
   int stat;   
   
   while( (pid = waitpid ( -1, &stat, WNOHANG)) > 0 )
    fprintf(stderr,"child %d terminated signo %d\n",pid,signo);  
    //return;
}
int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;/* portno-- stores the port number on which the server accepts connections */
     socklen_t clilen;/*clilen stores the size of the address of the client*/
    
     pid_t pid;
     
     struct sockaddr_in serv_addr, cli_addr;/*containing an internet address. */
     int n;/*it contains the number of characters read or written*/
     
     if (argc < 2) 
     {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
     sockfd = socket(AF_INET, SOCK_STREAM, 0);/* AF_INET is internet domain*/
     
     if (sockfd < 0) 
        error("ERROR opening socket");
        
     bzero((char *) &serv_addr, sizeof(serv_addr));/*sets all values in a buffer to zero. It takes two arguments, */
     												/*the first is a pointer to the buffer and the second is the */
     												/*size of the buffer. Thus, this line initializes serv_addr to zeros*/
     portno = atoi(argv[1]);/*The port number on which the server will listen for connections is passed in as an argument*/
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;/*This field contains the IP address of the host. For server code, */
     										/*this will always be the IP address of the machine on which the server is running,*/
     										/* and there is a symbolic constant INADDR_ANY which gets this address*/
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) /*binds a socket to an address, in this case */
     																		  /*the address of the current host and port number*/
     																		  /* on which the server will run*/
     											/*it takes three arguments, the socket file descriptor, the address to which*/
     											/*is bound, and the size of the address to which it is bound. The second */
     											/*argument is a pointer to a structure of type sockaddr, but what is passed in*/ 
     										/*is a structure of type sockaddr_in, and so this must be cast to the correct type*/
     	error("ERROR on binding");
     
     listen(sockfd,5);/*allows the process to listen on the socket for connections*/
     clilen = sizeof(cli_addr);
     
     //signal(SIGCHLD,SigCatcher);/* Handling Zombie Processes*/
     
     while(1)
     {
     	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,  &clilen);/*The accept() system call causes the process to block*/
     																	 /* until a client connects to the server. Thus, it wakes*/ 
     																	 /*up the process when a connection from a client has been*/ 
     																	 /*successfully established. It returns a new file */
     																	 /*descriptor, and all communication on this connection*/ 
     																	 /*should be done using the new file descriptor*/
     																	 /*The second argument is a reference pointer to the*/
     																	 /* address of the client on the other end of the */
     																	 /*connection, and the third argument is the size of */
     																	 /*this structure. */
     	printf("%d\n",newsockfd);
     	if (newsockfd < 0) 
          error("ERROR on accept"); 
          
        pid = fork();
   		if (pid < 0)
     		error("ERROR on fork");
   		if (pid == 0)
   		{
     		close(sockfd);
     		dostuff(newsockfd);
     		exit(0);
		} 
		else
     		close(newsockfd);
          
     
     }
     return 0; 
}
