#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/wait.h>
#define TCP1 4232
#define TCP2 4332
#define HOST "nunki.usc.edu"
#define BACKLOG 10
int processNumber;
pid_t procId;
int xCoordinate, yCoordinate;
char *name;
//struct to store coordinate
struct coord{
	int x;
	int y;
};
struct coord own;
//function to read data from file
void getData(int lineNumber, FILE *stream,struct coord *c,fpos_t *pos);
main(){
  procId = fork();

  if (procId == 0){
        name = "antenna1";
        processNumber = 1;
        }
  else {

        name = "antenna2";
        processNumber = 2;

        }
  
  sleep(10);
  FILE *fp;

  	if ((fp = fopen("scenario.txt", "r")) == NULL) {
  		printf("Couldn't open scenario.txt for reading\n");
  		exit(1);
  	}
  	fpos_t pos;
  	fgetpos(fp, &pos);   // save the position at the beginning of the file

  	//read data from file
  	if (processNumber ==1 ){
  		getData(0, fp, &own, &pos);
  		} 
  	else {
  		getData(1, fp, &own, &pos);
  		}
  	sleep(1);
  	//taken from Beej guide
  	struct hostent *h;
	if ((h=gethostbyname(HOST)) == NULL) {  // get the host info
		herror("gethostbyname");
		exit(1);
	}
	int portNum = TCP1;
	if (processNumber==2) portNum = TCP2;
	printf("Antenna %d has TCP port %d and IP address \"%s\"\n",processNumber, portNum, inet_ntoa(*((struct in_addr *)h->h_addr)));

    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr;    // my address information
    struct sockaddr_in their_addr; // connector's address information
    int sin_size;
    //taken from Beej's guide
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0))==-1) {// do some error checking!
    	perror("socket");
    	exit(1);
    }
    my_addr.sin_family = AF_INET;         // host byte order
    if (processNumber==1){
    my_addr.sin_port = htons(TCP1);     // short, network byte order
    }else my_addr.sin_port = htons(TCP2);     // short, network byte order
    my_addr.sin_addr= *((struct in_addr *)h->h_addr);
    memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
    //binding socket descriptor with a portNumber
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr)==-1){
		printf("Couldn't bind a socket to a port number\n");
		exit(1);
	}
    //listening on that port number
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    socklen_t sin_size1;
    sin_size1 = sizeof their_addr;
   // accepting the incoming connection 
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size1);

    printf("Antenna %d is now connected to RP rover\n", processNumber);
    char bufer2[100];
    int numb2;

	   
    if ((numb2=recv(new_fd, bufer2, 99, 0)) == -1) {
        perror("recv");
        exit(1);
     }
    bufer2[numb2] = '\0';

 //printf("Antenna %d Received: %s\n",processNumber, bufer2);
   //parsing the packet received
  char space[] = " ";
  	char *r = strtok( bufer2, space );
  	char *r1 = strtok(NULL, space);  	
  	int rN = atoi(r1);
  	char *type = strtok(NULL, space);
  	char *numOfNeg = strtok( NULL, space );
  	int n = atoi(numOfNeg);
  	char *dx = strtok( NULL, space );
  	int delx = atoi(dx);
  	char *dy = strtok( NULL, space );
  	int dely = atoi(dy);
  	int rpx, rpy;
  	//determining the sign of the numbers received
  	if (n==0) {rpx= own.x + delx; rpy= own.y + dely;}
  	if (n==1) {rpx= own.x - delx; rpy = own.y + dely;}
  	if (n==2) {rpy =  own.y - dely; rpx= own.x + delx;}
  	if (n==3) {rpy =  own.y - dely; rpx= own.x - delx;}
  	//building a new packet
  	char frame1[30];  
  	char * flag1 = " Data ";
  	strcpy(frame1,""); 
  	char * h1 = "antenna ";  	
  	char antNum[5];  	
  	sprintf(antNum, "%d", processNumber);
  	char frame2[30];
  	char d1[5], d2 [5];
  	sprintf(d1, "%d", rpx);
  	sprintf(d2, "%d", rpy);
  	strcat(frame1, h1);   	
  	strcat(frame1, antNum);  	
  	strcat(frame1, flag1);
  	strcpy(frame2, frame1);
  	strcat(frame1, d1);  	
  	strcat(frame1, " ");  	
  	strcat(frame1, d2);  
  	//printf("Antenna %d Frame = %s\n",processNumber, frame1);
  	
  	//sending the frame with the coordinates of RP rover
	if (send(new_fd, frame1, strlen(frame1), 0) == -1){
					 perror("send");
					 exit(1);
	}
	//receiving request for content   
    if ((numb2=recv(new_fd, bufer2, 99, 0)) == -1) {
        perror("recv");
        exit(1);
     }
    bufer2[numb2] = '\0';
    //buliding a packet "anntenna 1 broadcasting"
    char pld [20];
    strcat(frame2, "A");
    strcat(frame2, antNum);
    strcat(frame2, "broadcasting");
    //printf("Broadcasting: %s\n", frame2);
    if (send(new_fd, frame2, strlen(frame2), 0) == -1){
    					 perror("send");
    					 exit(1);
    	}

  //printf("Antenna %d Received: %s\n",processNumber, bufer2);
  
  printf("Antenna %d: Completed transfer of TCP Packets to RP rover\n", processNumber);
  	close(new_fd);
    close(sockfd);
  }
void getData(int lineNumber, FILE *stream,struct coord *c,fpos_t *pos){
	fsetpos(stream, pos);
	char s [16];
	int i = lineNumber;
	char ch;
	while (i!=0){
		while (ch!='\n'){
			ch=fgetc(stream);
		}
		i=i-1;
		ch='a';
	}
	fgets(s,sizeof(s),stream);
	int len = strlen(s);

	char delims[] = " ";
	char *result1 = strtok( s, delims );
	char *result2 = strtok(NULL, delims);
	c->x = atoi(result2);
	char *result3 = strtok( NULL, delims );
	c->y = atoi(result3);
	
}

                              

 



            
             
