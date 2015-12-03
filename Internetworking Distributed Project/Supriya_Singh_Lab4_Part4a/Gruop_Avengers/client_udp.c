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
#include <pthread.h>
//#include "my570list.h"
//#include "cs570.h"

#define PKTSIZE 1448
#define QUEUESIZE 10000

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif /* ~TRUE */

#define portNo1 51717
#define portNo2 51718
#define portNo3 51719


//all function declarations
void *acceptAck(void *ptr);
void *retransmitAck(void *ptr);
void error(const char *);

// List operations

typedef struct tagMy570ListElem {
    void *obj;
    struct tagMy570ListElem *next;
    struct tagMy570ListElem *prev;
} My570ListElem;

typedef struct tagMy570List {
    int num_members;
    My570ListElem anchor;

    /* You do not have to set these function pointers */
    int  (*Length)(struct tagMy570List *);
    int  (*Empty)(struct tagMy570List *);

    int  (*Append)(struct tagMy570List *, void*);
    int  (*Prepend)(struct tagMy570List *, void*);
    void (*Unlink)(struct tagMy570List *, My570ListElem*);
    void (*UnlinkAll)(struct tagMy570List *);
    int  (*InsertBefore)(struct tagMy570List *, void*, My570ListElem*);
    int  (*InsertAfter)(struct tagMy570List *, void*, My570ListElem*);

    My570ListElem *(*First)(struct tagMy570List *);
    My570ListElem *(*Last)(struct tagMy570List *);
    My570ListElem *(*Next)(struct tagMy570List *, My570ListElem *cur);
    My570ListElem *(*Prev)(struct tagMy570List *, My570ListElem *cur);

    My570ListElem *(*Find)(struct tagMy570List *, void *obj);
} My570List;

//Returns the number of elements in the list
int  My570ListLength(My570List *list)
{
	return list->num_members;
}

//Returns TRUE if the list is empty, returns FALSE otherwise
int  My570ListEmpty(My570List *list)
{
	if(list->num_members<=0)
		return TRUE;
	else 
		return FALSE;
}

//Add obj after Last(). This function returns TRUE if the operation is performed successfully and returns FALSE otherwise
int  My570ListAppend(My570List *list, void *obj)
{
	My570ListElem *elem=(My570ListElem *) malloc(1 * sizeof (My570ListElem));
	//printf("Appending element...\n");
  elem->prev=list->anchor.prev;
  elem->next=&list->anchor;
  list->anchor.prev->next=elem;
  list->anchor.prev=elem;
	elem->obj=obj;
	list->num_members++;
	return TRUE;
}

//Unlink and delete elem from the list. Please do not delete the object pointed to by elem
void My570ListUnlink(My570List *list, My570ListElem *elem)
{
 	elem->prev->next = elem->next;
  elem->next->prev = elem->prev;
  elem->prev = elem->next = elem;      //change later
  free(elem);
  list->num_members--;
}

//Returns the first list element
My570ListElem *My570ListFirst(My570List *list)
{
	if(My570ListEmpty(list))
		return NULL;
	else
		return list->anchor.next;
}

//Returns the last list element
My570ListElem *My570ListLast(My570List *list)
{
	if(My570ListEmpty(list))
	{	
		//printf("Last() returned null.\n");
		return NULL;
	}
	else
	{
		//printf("Last() returned anchor.prev.\n");
		return list->anchor.prev;
	}
}

//Returns elem->next
My570ListElem *My570ListNext(My570List *list, My570ListElem *elem)    
{
//	if(elem->next==list->anchor)
	if(My570ListEmpty(list))
		return NULL;
	else if(elem->next==&(list->anchor))
		return NULL;
	else
		return elem->next;
}

//Returns elem->prev
My570ListElem *My570ListPrev(My570List *list, My570ListElem *elem)
{
	//if(elem->prev==list->anchor)
	if(My570ListEmpty(list))
		return NULL;
	else if(elem->prev==&(list->anchor))
		return NULL;
	else
		return elem->prev;
}

//Initialize the list. Returns TRUE if all is well and returns FALSE if there is an error initializing the list
int My570ListInit(My570List *list)
{
	//printf("Initializing...\n");
	memset(&list->anchor, 0, sizeof(My570ListElem));
	list->num_members=0;
	list->anchor.next=&list->anchor;  								//is this correct?
	list->anchor.prev=&list->anchor;
	list->anchor.obj=NULL;
	return TRUE;
}

//end of list operations

//all structure declarations
typedef struct PacketData 
{
	long seqNo;									//sequence number of the packet	//8
	size_t dataLength;							//length of the packet			//8
	unsigned char data[PKTSIZE];				//data from the file			//1448
	int retransmit;								//how many times retransmitted  //4
}Packet;

typedef struct unAckPackets{
        int seqNo;					
		double ts;
}unAckPacket;


int normalPacketSize; 	//size of all packets except maybe the last packet
int totalPackets;		//total number of packets
int ackPackets;
int lastPacketSize;		//size of the last packet
double packetTimestamps[QUEUESIZE+1];
Packet toSend,toReSend;
char hostName[100],portNo[50];
int sock1,sock2,sock3; 
unsigned int length;
struct sockaddr_in server1, server2, server3;
double t1,t2,t3,duration;	
struct timeval start,end,now;
FILE *fp;
int done;			//done transmission of entire file
char fileName[50];
My570List packetList;			//the master list of all unacknowledged packets
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;		//lock for packetList	
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;		//lock for file

//debugger funtion to print from the master list of unack packets
void printList()
{
	My570ListElem *elem=NULL;
	int i=0;
	for (elem=My570ListFirst(&packetList); elem != NULL; elem=My570ListNext(&packetList, elem)) 
	{
		unAckPacket *p1=(unAckPacket*)(elem->obj);
		printf("%d\n",p1->seqNo);	//printing sequence number	
	}
}

int main(int argc, char *argv[])
{
	int n;
	struct sockaddr_in from;
	struct hostent *hp;
	char buffer[1500];
	int currentSeqNo=0;
	double dif;
	totalPackets=0;
	ackPackets=0;
	lastPacketSize=0;
	pthread_t thread1, thread2;
	memset(&packetList, 0, sizeof(My570List));
	(void)My570ListInit(&packetList);
	done=0;
	if (argc != 3) 
	{ 
		printf("Usage: server filename\n");
		exit(1);
	}
	//printf("%d\n",sizeof(Packet));
	sock1=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock1 < 0) error("socket");
	
	int sock_buf_size=190000000;
	setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));
	setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));

	hp = gethostbyname(argv[1]);
	if (hp==0) error("Unknown host");
	length=sizeof(struct sockaddr_in);
	
	server1.sin_family = AF_INET;
	bcopy((char *)hp->h_addr, 
		(char *)&server1.sin_addr,
		 hp->h_length);
	server1.sin_port = htons(portNo1);

	//open the 1GB file
	size_t result;
	long fileSize;
	normalPacketSize = PKTSIZE;

	strcpy(fileName,argv[2]);
	fp = fopen (fileName,"r");
	if (fp==NULL) 
	{
		fputs ("File error",stderr); 
		exit (1);
	}
	
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	
	totalPackets = fileSize/normalPacketSize;
	if(fileSize%normalPacketSize>0)
	{
		totalPackets++;
		lastPacketSize=fileSize%normalPacketSize;
	}

	//send the file name and total number of packets to the server
	
	//create the initial packet
	memset(toSend.data,'\0',sizeof(toSend.data));
	toSend.seqNo=-1;
	int temp=sprintf(toSend.data,"%s:%ld",fileName,fileSize);
	toSend.dataLength=temp;
	toSend.retransmit=0;
	
	//send the initial packet 6 times
	int count=0;
	for(count=0;count<6;count++)
	{
		n=sendto(sock1,(void*)&toSend,
				sizeof(Packet),0,(const struct sockaddr *)&server1,length);

		if (n < 0) error("Sendto");
	}
	
	//wait for ack for the initial packet
	int initialSent=0;
	n = recvfrom(sock1,buffer,256,0,(struct sockaddr *)&from, &length);
	if (n < 0) error("recvfrom");
	else initialSent=1;

	printf("Sending file %s to the server...\n", fileName);
	
	//creating 2 threads, for accepting ACK and retransmitting lost packets
	int  iret1, iret2;
	int i1,i2;
	i1 = 1;
	i2 = 2;
	strcpy(hostName,argv[1]);
	//strcpy(portNo,argv[2]);
	iret1 = pthread_create( &thread1, NULL, acceptAck,(void*) i1);	//this thread for acknowledging packets
	
	gettimeofday(&start, NULL);
	t1=((start.tv_sec*1000000.0)+start.tv_usec)/1000;	

	//send all packets except the last packet
	int first =1;
	while(currentSeqNo<totalPackets-1)
	{
		//Create a packet
		//initialize the struct members
		memset(toSend.data,0,sizeof(toSend.data));
		toSend.seqNo=currentSeqNo;
		toSend.dataLength=normalPacketSize;
		toSend.retransmit=0;
		currentSeqNo++;
		
		//copy the packet from file into the buffer:
		pthread_mutex_lock(&mutex2);
		int read=0;
		while(!read)
		{
			bzero(buffer,sizeof(buffer));
			result = fread (buffer,1,normalPacketSize,fp);
			if (result != normalPacketSize) 
			{
				fputs ("Reading error",stderr); 
				continue;
			}
			else
				read=1;
		}
		pthread_mutex_unlock(&mutex2);
		//buffer[normalPacketSize]='\0';
		memcpy (toSend.data,buffer,normalPacketSize);
		//strcpy(toSend.data,buffer);
		
		//if(toSend.seqNo==0 || toSend.seqNo==totalPackets-2)			//print the first packet and second last
			//printf("Packet no. %ld \n\n %s\n\n---------------\n\n",toSend.seqNo,toSend.data);

		gettimeofday(&start, NULL);
		double sendTime=((start.tv_sec*1000000.0)+start.tv_usec)/1000;
	

		n=sendto(sock1,(void*)&toSend,
			sizeof(Packet),0,(const struct sockaddr *)&server1,length);

		if (n < 0) error("Sendto");

		unAckPacket *p1=malloc(sizeof(unAckPacket));	
		p1->seqNo=currentSeqNo-1;
		p1->ts=sendTime;
		pthread_mutex_lock(&mutex1);

		(void)My570ListAppend(&packetList,(void *)p1);	
		pthread_mutex_unlock(&mutex1);
		
		//start the re-transmission thread after the first packet has been sent
		if(first)
		{
			iret2 = pthread_create( &thread2, NULL, retransmitAck, (void*) i2);	//this thread for re-transmitting lost packets	
			first=0;
		}		
		int i;
		struct timespec ts;
		/* Delay for a bit */
        ts.tv_sec = 0;
        ts.tv_nsec = 10;
		nanosleep (&ts, NULL); 
	}
	
	//send the last packet
	//Create the last packet
	//initialize the struct members
	memset(toSend.data,0,sizeof(toSend.data));
	toSend.seqNo=currentSeqNo;
	toSend.dataLength=lastPacketSize;
	toSend.retransmit=0;
	
	//copy the packet from file into the buffer:
	pthread_mutex_lock(&mutex2);
	int read=0;
	while(!read)
	{
		bzero(buffer,sizeof(buffer));
		result = fread (buffer,1,lastPacketSize,fp);
		if (result != lastPacketSize) 
		{
			fputs ("Reading error",stderr); 
			continue;		
		}
		else
			read=1;
	}
	pthread_mutex_unlock(&mutex2);
	//buffer[lastPacketSize]='\0';
	memcpy (toSend.data,buffer,normalPacketSize);
	//strcpy(toSend.data,buffer);
	//printf("Packet no. %ld \n\n %s\n\n---------------\n\n",toSend.seqNo,toSend.data);
	gettimeofday(&start, NULL);
	double sendTime=((start.tv_sec*1000000.0)+start.tv_usec)/1000;
	
	n=sendto(sock1,(void*)&toSend,
			sizeof(Packet),0,(const struct sockaddr *)&server1,length);	//sent the last packet

	if (n < 0) error("Sendto");
	
	//add this packet to the unack packet list
	unAckPacket *p1=malloc(sizeof(unAckPacket));	
	p1->seqNo=currentSeqNo;
	p1->ts=sendTime;
	pthread_mutex_lock( &mutex1 );

	(void)My570ListAppend(&packetList,(void *)p1);	

	pthread_mutex_unlock( &mutex1 );
	//printf("Done sending\n");
    pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	printf("File transferred successfully.\n");
	//printf("Thread 1 returns: %d\n",iret1);
	//printf("Thread 2 returns: %d\n",iret2);	
	fclose(fp);
	close(sock1);
	return 0;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//this function is to accept the packet acknowledgements sent by server
//it removes the ack packets from re-transmission queue
void *acceptAck(void *ptr)
{
	int n;
	struct sockaddr_in from;
	char buffer[1500];
	//printf("Thread 1 executing!!! :) :)\n");	
	int lastSeqNo=0;
	struct timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	while(ackPackets!=totalPackets)
	{
		n = recvfrom(sock1,buffer,256,0,(struct sockaddr *)&from, &length);
		if (n < 0) error("recvfrom");

		int ackPktNo=atoi(buffer);
		
		if(ackPktNo==-1)	//this is just a repeat initial packet ack
			continue;
		if(ackPktNo==-2)	//this is an indication that server is done
		{
			ackPackets=totalPackets;
			//printf("Server done!\n");
			//nanosleep (&ts, NULL);		/* Delay for a bit */
			break;
		}
			
		//remove packet from retransmission queue
		My570ListElem *elem=NULL;
		int i=0;
		
		pthread_mutex_lock(&mutex1);
		for (elem=My570ListFirst(&packetList); elem != NULL; elem=My570ListNext(&packetList, elem)) 
		{
			unAckPacket *p1=(unAckPacket*)(elem->obj);
			if(p1->seqNo==ackPktNo)
			{
				My570ListUnlink(&packetList, elem);
				ackPackets++;	
				break;
			}
		}
		pthread_mutex_unlock(&mutex1);
	}
	pthread_mutex_lock(&mutex1);
	done=1;
	pthread_mutex_unlock(&mutex1);
	gettimeofday(&end, NULL);	
	t2=((end.tv_sec*1000000.0)+end.tv_usec)/1000;
	duration = t2 - t1;
	printf ("RTT was %lf ms\n",duration);
}

//retransmit all packets in queue

void *retransmitAck(void *ptr)
{
	char buffer[1500];
	int result,n;
	struct timespec ts;
	/* Delay for a bit */
	ts.tv_sec = 0;
	ts.tv_nsec = 400000000;
	nanosleep (&ts, NULL);		//400 ms wait before starting to retransmit to give the sent packets a good headstart
	ts.tv_sec = 0;
	ts.tv_nsec = 10;	
	//printf("Thread 2 executing!!! :D :D\n");
	double nowTime;
	pthread_mutex_lock(&mutex1);
	while(done==0)
	{
		My570ListElem *elem=NULL;
		elem=My570ListFirst(&packetList); 
		if(elem != NULL)
		{
			unAckPacket *p1=(unAckPacket*)(elem->obj);
			memset(toReSend.data,'\0',sizeof(toReSend.data));
			toReSend.seqNo=p1->seqNo;
			double pktTS=p1->ts;
			pthread_mutex_unlock(&mutex1);
			gettimeofday(&now, NULL);
			nowTime=((now.tv_sec*1000000.0)+now.tv_usec)/1000;	
			if(nowTime-pktTS>400)				//if timeout then resend
			{
				//printf("%lf %lf\n",nowTime,pktTS);
				int offset=toReSend.seqNo*PKTSIZE;
				pthread_mutex_lock(&mutex2);
				int temp=ftell(fp);
				fseek(fp,offset,SEEK_SET);
				if(toReSend.seqNo==totalPackets-1)
				{
					//last Packet
					int read=0;
					while(!read)
					{
						result = fread (buffer,1,lastPacketSize,fp);
						if (result != lastPacketSize) 
						{
							fputs ("Reading error",stderr); 
							continue;
						}
						else
							read=1;
					}
					buffer[lastPacketSize]='\0';
					toReSend.dataLength=lastPacketSize;
				}
				else
				{
					//normal packet
					int read=0;
					while(!read)
					{
						bzero(buffer,sizeof(buffer));
						result = fread (buffer,1,normalPacketSize,fp);
						if (result != normalPacketSize) 
						{
							fputs ("Reading error",stderr); 
							continue;
						}
						else
							read=1;
					}
					buffer[normalPacketSize]='\0';
					toReSend.dataLength=normalPacketSize;
				}
				fseek(fp,temp,SEEK_SET);
				pthread_mutex_unlock(&mutex2);
				memcpy (toReSend.data,buffer,normalPacketSize);
				//strcpy(toReSend.data,buffer);			
			
				toReSend.retransmit=1;
				gettimeofday(&now, NULL);
				nowTime=((now.tv_sec*1000000.0)+now.tv_usec)/1000;
				n=sendto(sock1,(void*)&toReSend,
						sizeof(Packet),0,(const struct sockaddr *)&server1,length);
				if (n < 0) error("Sendto");
				//printf("Message retransmitted: %d\n",toReSend.seqNo); 
				pthread_mutex_lock(&mutex1);
				My570ListUnlink(&packetList, elem);
				unAckPacket *p1=malloc(sizeof(unAckPacket));	
				p1->seqNo=toReSend.seqNo;
				p1->ts=nowTime;
				(void)My570ListAppend(&packetList,(void *)p1);				
				pthread_mutex_unlock(&mutex1);
			}
		}	
		else
		{	
			pthread_mutex_unlock(&mutex1);
		}
		nanosleep (&ts, NULL);
		pthread_mutex_lock(&mutex1);
	}
	pthread_mutex_unlock(&mutex1);
}

