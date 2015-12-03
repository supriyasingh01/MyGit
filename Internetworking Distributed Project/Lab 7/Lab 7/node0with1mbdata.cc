//Client Code for Node_0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> /*Contains datatypes used in system calls*/
#include <sys/socket.h>/* contains structures needed for sockets*/
#include <netinet/in.h> /* ile in.h contains constants and structures needed for internet domain addresses. */
#include <netdb.h> /*defines the structure hostent*/

using namespace std;

//MACROS
#define PKTSIZE 100

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
int lastPacketSize;		//size of the last packet

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int clientSocket, portno, n;
    struct sockaddr_in serv_addr;/*contain the address of the server to which we want to connect*/
    struct hostent *server;
    int numOfDataSend=0;
      
    if (argc < 3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    portno = atoi(argv[2]);
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (clientSocket < 0) 
        error("ERROR opening socket");
        
    server = gethostbyname(argv[1]);/*Takes such a name as an argument and returns a pointer to a hostent containing information*/
    								/* about that host. The field char *h_addr contains the IP address*/
    
    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,  (char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);
    
    if (connect(clientSocket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) /*to establish a connection to the server*/
    																		/*t takes three arguments, the socket file */
    																		/*descriptor, the address of the host to which it*/
    																		/* wants to connect (including the port number), and*/
    																		/* the size of this address. This function returns 0*/
    																		/* on success*/
    																		/*he client needs to know the port number of */
    																		/*the server, but it does not need to know its own */
    																		/*port number. This is typically assigned by the*/ 
    																		/*system when connect is called*/
        error("ERROR connecting");
        
    
    unsigned char buffer[1500];// This is used for storing data that is read using fread. 
    for(int i=0;i<1500;i++)
		buffer[i] = '\0';
		
	int currentSeqNo=0;
	totalPackets=0;
	lastPacketSize=0;
    
	//open the file
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

	totalPackets = fileSize/normalPacketSize;
	if(fileSize % normalPacketSize>0)//
	{
		totalPackets++;// Adding the last packet
		lastPacketSize=fileSize % normalPacketSize;// last Packet
	}
	//.................................FILE SEND START.....................................................................//
	printf("Total no. of Packets %d\n",totalPackets);// for 100M it is 1048576
	
	int dealyCount = 0;
	
	//send all packets except the last packet
	while(currentSeqNo <=totalPackets-1)
	{
		currentSeqNo++;
		//printf("seq no. %d\n",currentSeqNo);
		//Create a packet initialize the struct members
		memset(toSend.data,'\0',sizeof(toSend.data));
		toSend.seqNo=currentSeqNo;
		toSend.dataLength = PKTSIZE-sizeof(long)-sizeof(size_t);
		
		//copy the packet from file into the buffer:
		result = fread (buffer,1,toSend.dataLength,fp);//Copy the data of size normalPacketSize each of 1 byte into buffer
		//printf("result =%d, toSend.dataLength =%d\n",result,toSend.dataLength);
		if (result != toSend.dataLength) 
		{
			fputs ("Reading error of normal packet",stderr); 
			//exit (3);
		}

		for(int i=0;i<toSend.dataLength;i++)
			toSend.data[i] = buffer[i];
		
		numOfDataSend=write(clientSocket,(void*)&toSend,normalPacketSize);
				if (numOfDataSend < 0) 
				{	
					error("ERROR writing to socket");	
					exit(0);
				}
		printf("numOfDataSend =%d whose seq no is %d\n",numOfDataSend,toSend.seqNo);
		
		usleep(999);
		//printf("Amount of Data Send is %d and sequence no. is %ld \n",numOfDataSend,toSend.seqNo);
	}
	//send the last packet Create the last packet initialize the struct members
	memset(toSend.data,'\0',sizeof(toSend.data));
	toSend.seqNo=currentSeqNo;
	toSend.dataLength=lastPacketSize-sizeof(long)-sizeof(size_t);
	
	//copy the packet from file into the buffer:
	result = fread (buffer,1,toSend.dataLength,fp);
	if (result != toSend.dataLength) 
	{
		fputs ("Reading error of last packet",stderr); 
		exit (3);
	}
	
	for(int i=0;i<toSend.dataLength;i++)
			toSend.data[i] = buffer[i];
	
	numOfDataSend=write(clientSocket,(void*)&toSend,(lastPacketSize + sizeof(long) + sizeof(size_t)));	//sent the last packet
	if (numOfDataSend < 0) error("Sendto");
	
	printf("Data in last packet is %d and it SeqNo, is %ld\n",numOfDataSend,toSend.seqNo);
	
	//.................................FILE SEND COMPLETE...............................................................//
	
	//Receiving message from server
	/*char recievedBuffer[32];
	for(int i=0;i<32;i++)
		recievedBuffer[i] = '\0';
		
    n = read(clientSocket,recievedBuffer,32); // Reading from router1 that that much amount data is received
    if (n < 0) 
         error("ERROR reading from socket");
         
    printf("These many no. of packtes recived = %s\n",recievedBuffer);*/
    
close(clientSocket);
return 0;
}
