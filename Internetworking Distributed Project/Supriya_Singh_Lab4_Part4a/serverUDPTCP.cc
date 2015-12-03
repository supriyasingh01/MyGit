/* Creates a datagram server.  The port 
   number is passed as an argument.  This
   server runs forever */

#include <sys/types.h>
#include <stdlib.h> 
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

//MACROS
#define PKTSIZE 1000
#define PORT "3490"  // the port users will be connecting to
#define BACKLOG 10     // how many pending connections queue will hold


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//Packet
typedef struct PacketData 
{
	long seqNo;									//sequence number of the packet of 8 bytes each
	size_t dataLength;							//length of the packet of 8 bytes each
	unsigned char data[PKTSIZE-sizeof(long)-sizeof(size_t)];	//data from the file of 1 byte each
}Packet;

int sock_server;// server socket descriptor
int length;// stores the length of server ip address
socklen_t fromlen;
struct sockaddr_in server;
FILE * pFile;
map<long,unsigned char*> mymap;// For storing data of file 
map<long,long> mylostmap;// For storing ACK andlost data of file 
map<long,unsigned char*>::iterator it;

int main(int argc, char *argv[])
{
	int n,m;// n is the data send/recv by server.  m is the amount of data in bytes written into the test file at one time
	struct sockaddr_in from;/// from is the client info
	
	
	if (argc < 2) {
	  fprintf(stderr, "ERROR, no port provided\n");
	  exit(0);
	} 
	
	sock_server=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_server < 0) 
		error("ERROR Opening UDP socket");  
    
	length = sizeof(server);
	bzero(&server,length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(atoi(argv[2]));
	if (bind(sock_server,(struct sockaddr *)&server,length)<0) 
	error("binding");

	

	fromlen = sizeof(struct sockaddr_in);
	pFile = fopen ("test","w+");
	
	long packetCount = 0;// For now its hard coeded  to 729444 from the value of client side
	long numberOfLostPkts = 0;
	long lostCount = 0;
	
	while (packetCount <= 1073741) 
	{
		Packet *dataReceived = NULL;
		dataReceived = (Packet*)malloc(sizeof(Packet));
		memset(dataReceived->data,'\0',sizeof(dataReceived->data));
		
		n = recvfrom(sock_server,dataReceived,sizeof(Packet),0,(struct sockaddr *)&from,&fromlen);
		//printf("%d bytes recievd by the server in for %d packet \n", n, dataReceived->seqNo);
		
		if (n < 0) error("recvfrom");
		
		fflush(stdout);
		//printf("Sequence no. is %ld of the packet that has been recieved and its length in bytes is %d bytes \n",dataReceived->seqNo,n);
		
		// Writing data that has been recieved into the map now and then later into file.
		if(packetCount == dataReceived->seqNo)
		{
			mymap[packetCount]=dataReceived->data;
			free(dataReceived);
			printf("Got right packet%ld\n",packetCount);
			packetCount++;
		}
		else if(packetCount < dataReceived->seqNo)
		{
			mymap[packetCount]=NULL;
			printf("lost pkts %ld\n",packetCount);
			mylostmap[lostCount] = packetCount;
			packetCount++;
			numberOfLostPkts++;
			lostCount++;
		}
		else(packetCount > dataReceived->seqNo);
		{
			// do nothing recieve next packet until u get different packet;
		}
		
		
	}
		 cout << "mymap.size() is " << (long) mymap.size() << endl;
		 printf("numberOfLostPkts pkts %ld\n",numberOfLostPkts);
		 cout << "mylostmap.size() is " << (long) mylostmap.size() << endl;
		
		
		
		long jp;
		jp=sendto(sock_server,"hello",strlen("hello"),0,(const struct sockaddr *)&from,fromlen);	
		
		
		
close(sock_server);				
		
		// Sending ACK of lost packets using TCP.
		
		int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     //int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[2]) +2;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
     close(newsockfd);
     close(sockfd);

// 		//Storing in a map the sequence number of lost packets suing UDP
// 		
// 		 //Send the ACK for los packets
// 		 long mpSiz = 0;
// 		 char ACK[12];
// 		 for(int i=0;i<12;i++)
// 		 	ACK[i] = '\0';
// 		 long lostData =0;
// 		 int lostDataBytes=0;
// 		 int result = 0;
// 		 
// 		 long mapLostDataSize = mylostmap.size();
// 		 
// 		 while(mpSiz <= mapLostDataSize)
// 		 {
// 		    lostData = mylostmap.find(mpSiz)->second;
// 		 	//Send the map key value for whose data is '\0'
// 		 		sprintf(ACK,"%ld",lostData);
// 		 		//sent the lost packet
// 		 		lostDataBytes=sendto(sock_server,ACK,strlen(ACK),0,(const struct sockaddr *)&from,length);
// 				result = usleep(999); 
// 		 		printf("Sedning ACK %ld \n",lostData);
// 		 		//Receive data from client
// 		 		//Store it in the map
// 		 	
// 		 	mpSiz++;
// 		 	
// 		  }
		 
	
fclose (pFile);
exit(0);
return 0;
}

