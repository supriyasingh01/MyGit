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
#include <pthread.h>
//#include "my570list.h"
//#include "cs570.h"

#ifndef TRUE
#define FALSE 0
#define TRUE 1
#endif /* ~TRUE */

#define PKTSIZE 1448
#define portNo1 51717
#define portNo2 51718
#define portNo3 51719

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void *fileWrite(void *ptr);

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

typedef struct PacketData 
{
	long seqNo;									//sequence number of the packet
	size_t dataLength;							//length of the packet
	unsigned char data[PKTSIZE];				//data from the file
	int retransmit;								//how many times retransmitted
}Packet;

int sock1,sock2,sock3;
int length;
socklen_t fromlen;
struct sockaddr_in server1,server2,server3;
FILE * pFile;
long int totalPackets;
long int receivedPackets;
int writtenPackets;
long fileSize;
int normalPacketSize; 	//size of all packets except maybe the last packet
int lastPacketSize;
double t1,t2,t3,duration;	
struct timeval start,end,now;
int first;	//first packet received, so start the filewrite thread
int done;

My570List packetList;			//the master list of all packets to be written to file
My570List lostPacketList;			//the master list of all lost packets
My570List repeatPacketList;			//the master list of all lost packets to check if any have been repeated
//My570List packetList;			//the master list of all packets to be written to file
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;		//lock for packetList	
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;		//lock for file


//debugger funtion to print from the master list of unack packets
void printList()
{
	My570ListElem *elem=NULL;
	int i=0;
	for (elem=My570ListFirst(&packetList); elem != NULL; elem=My570ListNext(&packetList, elem)) 
	{
		Packet *p1=(Packet*)(elem->obj);
		printf("%ld\t",p1->seqNo);	//printing sequence number	
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int n,m;
	struct sockaddr_in from;
	char buf[PKTSIZE];
	char writePkt[2000];
	pthread_t thread1;
	receivedPackets=0;
	writtenPackets=0;
	done =0;
	normalPacketSize = PKTSIZE;
	first=0;
	int repeat=0;
	
	memset(&packetList, 0, sizeof(My570List));
	(void)My570ListInit(&packetList);
	
	memset(&lostPacketList, 0, sizeof(My570List));
	(void)My570ListInit(&lostPacketList);	
	memset(&repeatPacketList, 0, sizeof(My570List));
	(void)My570ListInit(&repeatPacketList);	
	
	sock1=socket(AF_INET, SOCK_DGRAM, 0);
	if (sock1 < 0) error("Opening socket");

	int sock_buf_size=190000000;
	setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));
	setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (char*)&sock_buf_size, sizeof(sock_buf_size));
	
	length = sizeof(server1);
	bzero(&server1,length);
	server1.sin_family=AF_INET;
	server1.sin_addr.s_addr=INADDR_ANY;
	server1.sin_port=htons(portNo1);
	if (bind(sock1,(struct sockaddr *)&server1,length)<0) 
		error("binding");

	fromlen = sizeof(struct sockaddr_in);

	//receive the initial packet
	Packet *initialPacket = NULL;
	initialPacket = malloc(sizeof(Packet));
	memset(initialPacket->data,0,sizeof(initialPacket->data));	
	n = recvfrom(sock1,initialPacket,sizeof(Packet),0,(struct sockaddr *)&from,&fromlen);
	if (n < 0) error("recvfrom");
	fflush(stdout);
	
	//send ack for initial packet 6 times
	sprintf(buf,"%ld",initialPacket->seqNo);
	int count=0;
	for(count=0;count<6;count++)
	{
		n = sendto(sock1,buf,256,0,(struct sockaddr *)&from,fromlen);
		if (n  < 0) error("sendto");	
	}
	
	//process initial packet
	char *fileName;
	char *totalPacketsStr;

	fileName = strtok (initialPacket->data,":");
	totalPacketsStr = strtok(NULL, ":");
	fileSize=atol(totalPacketsStr);
	totalPackets = fileSize/normalPacketSize;
	if(fileSize%normalPacketSize>0)
	{
		totalPackets++;
		lastPacketSize=fileSize%normalPacketSize;
	}	
	//printf ("Filename: %s, Total Packets: %d, Last Packet Size: %d\n",fileName,totalPackets,lastPacketSize);
	printf ("Receiving file %s...\n",fileName);
	pFile = fopen (fileName,"w+");
	//pFile = fopen ("test","w+");
	
	int lastSeqNo=-1;
	gettimeofday(&start, NULL);
	t1=((start.tv_sec*1000000.0)+start.tv_usec)/1000;	
	
	//start receiving packets
	while (receivedPackets<totalPackets) 
	{
		repeat=1;
		Packet *dataReceived = malloc(sizeof(Packet));
		//memset(dataReceived->data,0,sizeof(dataReceived->data));
		n = recvfrom(sock1,dataReceived,sizeof(Packet),0,(struct sockaddr *)&from,&fromlen);
		if (n < 0) error("recvfrom");
		fflush(stdout);
		if(dataReceived->seqNo<0)	//this is just a repeat initial packet
			continue;		
			
		//check if this is a repeat packet
		long int check=dataReceived->seqNo;
		if(check>lastSeqNo)
			repeat=0;
			
		My570ListElem *elem=NULL;
		for (elem=My570ListFirst(&repeatPacketList); elem != NULL; elem=My570ListNext(&repeatPacketList, elem)) 
		{
			long int ival=(long int)(elem->obj);
			if(ival==check)
			{
				repeat=0;				//this is a lost packet
				//printf("unlinking %d\n",ival);			
				My570ListUnlink(&repeatPacketList, elem);
				break;
			}
		}
		
		if(!repeat && dataReceived->retransmit==0)
		{
			//find lost packets and append to the lost packet list
			int i;
			if(dataReceived->seqNo>0 && dataReceived->seqNo!=lastSeqNo+1 && dataReceived->seqNo>lastSeqNo)
			{
				//printf("Packets lost:\n");
				i=lastSeqNo;
				while(i<dataReceived->seqNo-1)
				{
					i++;
					//printf("%d\t",i);
					(void)My570ListAppend(&lostPacketList, (void*)i);
					(void)My570ListAppend(&repeatPacketList, (void*)i);
				}
				//printf("\n");
			}
			lastSeqNo=dataReceived->seqNo;
		}

		
		//send ack
		sprintf(buf,"%ld",dataReceived->seqNo);
		n = sendto(sock1,buf,256,0,(struct sockaddr *)&from,fromlen);
		if (n  < 0) error("sendto");

		if(!repeat)	
		{
			receivedPackets++;
		
			//add received packet to the list
			
			pthread_mutex_lock(&mutex1);
			(void)My570ListAppend(&packetList,(void *)dataReceived);	
			pthread_mutex_unlock(&mutex1);
		}
		
		//create file write thread if received packet is the first packet
		if(first==0)
		{
			//create a thread here
			int iret1,i1;
			iret1 = pthread_create(&thread1, NULL, fileWrite,(void*) i1);	//this thread for writing to file
			first=1;
		}			
	}
	struct timespec ts;
	ts.tv_sec = 2;
	ts.tv_nsec = 0;	
	count=0;
	//printf("Sending the EOF\n");
	for(count=0;count<6;count++)
	{
		//send ack
		sprintf(buf,"%d",-2);
		n = sendto(sock1,buf,256,0,(struct sockaddr *)&from,fromlen);
		//printf("n:%d\n",n);
		if (n  < 0) {break;}
	}
	//nanosleep (&ts, NULL);		/* Delay for a bit */
	gettimeofday(&end, NULL);	
	t2=((end.tv_sec*1000000.0)+end.tv_usec)/1000;
	duration = (t2 - t1)/1000;
	printf ("File transferred %lf seconds.\n",duration);
	pthread_mutex_lock(&mutex1);
	done=1;
	pthread_mutex_unlock(&mutex1);
	pthread_join(thread1, NULL);
	//fclose (pFile);
	//printf("Exiting main\n");
	close(sock1);
	return 0;
 }

 
 void *fileWrite(void *ptr)
{
	//printf("Thread 1 executing!!! :) :)\n");
	
	//keep reading from list and writing to file
	pthread_mutex_lock(&mutex1);
	while(done==0 || My570ListLength(&packetList)>0)
	{
		//take a packet from the list
		My570ListElem *elem=NULL;
		elem=My570ListFirst(&packetList); 
		if(elem != NULL)
		{
			Packet *d1=(Packet*)(elem->obj);
			pthread_mutex_unlock(&mutex1);
			int written=0;	
			while(!written)
			{
				int offset=d1->seqNo*PKTSIZE;
				fseek(pFile,offset,SEEK_SET);
				int m;
				m=fwrite(d1->data,1,d1->dataLength,pFile);  
				if (m != d1->dataLength) 
				{
					fputs ("Writing error",stderr); 
					continue;
				}
				else 
				{
					written=1;
					fflush (pFile);
				}
			}
			writtenPackets++;
			pthread_mutex_lock(&mutex1);
			My570ListUnlink(&packetList, elem);
			pthread_mutex_unlock(&mutex1);
		}
		else
		{	
			pthread_mutex_unlock(&mutex1);
		}
		pthread_mutex_lock(&mutex1);
	}
	pthread_mutex_unlock(&mutex1);
	//printf("Exiting thread1 %d\n",writtenPackets);
	fclose(pFile);
}
