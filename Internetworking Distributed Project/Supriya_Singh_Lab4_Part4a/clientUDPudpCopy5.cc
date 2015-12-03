/* UDP client in the internet domain */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <map>

using namespace std;

//MACROS
#define PKTSIZE 1000

//all function declarations
void *acceptAck(void *ptr);
void *retransmitAck(void *ptr);
void error(const char *);

// FOR TCP
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//all structure declarations
typedef struct PacketData 
{
	long seqNo;									//sequence number of the packet is of 8 bytes each. FORMAT SPECIFIER is ld
	size_t dataLength;							//length of the packet is of 8 bytes each. FORMAT SPECIFIER is zd
	unsigned char data[PKTSIZE-sizeof(long)-sizeof(size_t)];	//data from the file is of 1 byte each. FORMAT SPECIFIER is ld
}Packet;

Packet toSend;

int normalPacketSize; 	//size is 1472 of all packets except the last packet
int totalPackets;		//total number of packets
int ackPackets;
int lastPacketSize;		//size of the last packet

int clientSocket; //client sockect
unsigned int length; // length of ip address
struct sockaddr_in server;

map<long,unsigned char*> mymap;// For storing data of file 
map<long,unsigned char*>::iterator it;
	
int main(int argc, char *argv[])
{
	int numOfDataSend = 0;// NO. of packets send
	struct sockaddr_in from;
	struct hostent *hp;
	unsigned char buffer[1500];// This is used for storing data that is read using fread. 
	int currentSeqNo=0;
	totalPackets=0;
	lastPacketSize=0;
    int n;
	
	for(int i=0;i<1500;i++)
		buffer[i] = '\0';
	
	if (argc != 3) 
	{ 
		printf("Usage: server port\n");
		exit(1);
	}
	
	clientSocket=socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket < 0) error("socket");
	hp = gethostbyname(argv[1]);
	if (hp==0) error("Unknown host");
	length=sizeof(struct sockaddr_in);
	server.sin_family = AF_INET;
	bcopy((char *)hp->h_addr, 
		(char *)&server.sin_addr,
		 hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	
	//open the 1GB file
	FILE *fp;// file pointer
	size_t result = 0;// used to store the value returned by the fread(that returns in bytes)
	long fileSize =0;
	normalPacketSize = PKTSIZE;

	fp = fopen ("data.bin","r");
	if (fp==NULL) 
	{
		fputs ("File error",stderr); 
		exit (1);
	}
	
	fseek(fp, 0L, SEEK_END);//fp is file pointer, 0L denotes the position of the file pointer, SEEK_END is the end of file
							// So fp now points to the end of the file
	fileSize = ftell(fp);// getting the fle size
	fseek(fp, 0L, SEEK_SET);// SEEK_SET is the beginning of the file

	totalPackets = fileSize/normalPacketSize;// 729444 are total no. packets 729444
	//printf("totalPackets %d normalPacketSize %d sizo of pkt %ld\n",totalPackets,normalPacketSize,sizeof(Packet) );
	
	
	if(fileSize % normalPacketSize>0)// fileSize 1073741824 is 1024 * 1024 *1024 bits = 1Gbbyes 
	{
		totalPackets++;// Adding the last packet
		lastPacketSize=fileSize % normalPacketSize;// last Packet
	}
	
	//send all packets except the last packet
	while(currentSeqNo < totalPackets-1)
	{
		//Create a packet initialize the struct members
		memset(toSend.data,'\0',sizeof(toSend.data));
		toSend.seqNo=currentSeqNo;
		toSend.dataLength = PKTSIZE-sizeof(long)-sizeof(size_t);
		
		
		
		//copy the packet from file into the buffer:
		result = fread (buffer,1,toSend.dataLength,fp);//Copy the data of size normalPacketSize each of 1 byte into buffer
		if (result != toSend.dataLength) 
		{
			fputs ("Reading error of normal packet",stderr); 
			exit (3);
		}
		//buffer[normalPacketSize]='\0';
		
		//strcpy(toSend.data,buffer);
		for(int i=0;i<toSend.dataLength;i++)
			toSend.data[i] = buffer[i];
		
		int numofSend=0;	
		while(numofSend<4)
		{
			numOfDataSend=sendto(clientSocket,(void*)&toSend,normalPacketSize,0,(const struct sockaddr *)&server,length);
				if (numOfDataSend < 0) error("Sendto");
			numofSend++;
		}
		
		printf("Amount of Data Send is %d and sequence no. is %ld \n",numOfDataSend,toSend.seqNo);
		
		//Storing file contents in a map
		mymap[currentSeqNo]=buffer;
		currentSeqNo++;
		
		/* Delay for a bit */
		/*struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 9000000000;
		nanosleep (&ts, NULL);*/
		int result = 0;
		result = usleep( 4 ); 
	}
	
	//send the last packet Create the last packet initialize the struct members
	memset(toSend.data,'\0',sizeof(toSend.data));
	toSend.seqNo=currentSeqNo;
	toSend.dataLength=lastPacketSize-sizeof(long)-sizeof(size_t);
	
	//Storing file contents in a map
		mymap[currentSeqNo]=buffer;

	//copy the packet from file into the buffer:
	result = fread (buffer,1,toSend.dataLength,fp);
	if (result != toSend.dataLength) 
	{
		fputs ("Reading error of last packet",stderr); 
		exit (3);
	}
	//buffer[lastPacketSize]='\0';
	
	
	for(int i=0;i<toSend.dataLength;i++)
			toSend.data[i] = buffer[i];
	
	int numofSend=0;	
	while(numofSend<2)
		{	
			numOfDataSend=sendto(clientSocket,(void*)&toSend,(lastPacketSize + sizeof(long) + sizeof(size_t)),0,(const struct sockaddr *)&server,length);	//sent the last packet
			if (numOfDataSend < 0) error("Sendto");
			numofSend++;
		}
	
	printf("Data in last packet is %d and it SeqNo, is %ld\n",numOfDataSend,toSend.seqNo);
	cout << "mymap.size() is " << (int) mymap.size() << endl;
	
	int jkl=0;
	char bufg[6];
	jkl = recvfrom(clientSocket,bufg,7,0,(struct sockaddr *)&server,&length);
	printf("%s\n",bufg);
	

	// Now sending lost data
	long mpSiz = 0;
	int lostData =0;
	long mapDataSize = mymap.size();
	//printf("%ld\n",mapDataSize);
	char ACK[12];// note earlier it was mistake .. u have to always pass charecter array to sento and recv.
	for(int i=0;i<12;i++)
		 	ACK[i] = '\0';
	while(mpSiz <= mapDataSize)
	{
		//Receive the ACK 
		sprintf(ACK,"%ld",mpSiz);
		lostData= recvfrom(clientSocket,ACK,12,0,(struct sockaddr *)&server,&length);
		printf("ACK of lostadata =%s\n",ACK);
		mpSiz++;
		printf("no of lost pkts %ld\n",mpSiz);
	}

	//retrieve the data from the map using the above ACK
	//Send the data of the 
	
close(clientSocket);	
return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}