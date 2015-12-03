#include <string.h>
//#include <cstdlib>
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
#include <math.h>
#define HOST "nunki.usc.edu"
#define MAXBUFLEN 100
#define UDP1  21232
#define UDP2  21332
#define UDP3  21432
#define UDP4  21532
#define UDP5  21632
#define UDP6  21732
#define UDP7  21832
#define TCP1 4232
#define TCP2 4332




struct coord{
	int x;
	int y;
};

struct coord own, antenna1, antenna2;
int distToA1, distToA2, totalDist;
char buf [MAXBUFLEN];
struct hostent *h;
int sockfd;
int RP; //stores number of rover who becomes Rendevouz Point
socklen_t addr_len;
int numbytes;
int processNumber; //own process Number assigned in the program
int receivedDist[8]; //stores the total Distance to the antenna for the rovers 
int a[7];//stores the numbers of rovers subscribed to antennas
char *c[7];//stores the number of antenna the rover subscribed to
struct hostent *he;
struct sockaddr_in thaddr1, thaddr2, myAddr;
int sockfd3,sockfd4, numb1, numb2;

int deltax2, deltay2, deltax1, deltay1;
char dx2[5], dx1[5];
char  dy2[5], dy1[5];
void getData(int lineNumber, FILE *stream, struct coord *c,fpos_t *pos);
int calculateDistance(struct coord * c);
void sendHelo(int port); //sends rover # Helo dist packet 
void receiveHelo(int port);// receives that Helo packet
void displayDist(); //used for debugging
bool compareDist();//compares dist, returns true if it is Rendezvous point
void sendPacket(int port, char *type, char *payload);//sends other UDP packets
void receivePacket(int port, int *rN, char **str); //receives the Packets

struct sockaddr_in sockAddress, theirAddress, destAddress;

main(){

  sleep(5);
	pid_t procId;
	procId = fork();
	if (procId == 0){
		processNumber =1;
		pid_t p1=fork();
		if (p1==0)
		{
			processNumber = 3;
			pid_t p2 = fork();
			if (p2==0){
				processNumber =4;
				pid_t p3 = fork();
				if (p3==0){
					processNumber =5;
				}
			}
		}
	}
	else {
		processNumber = 2;
		pid_t p4 = fork();
		if (p4==0){
			processNumber = 6;
			pid_t p5 = fork();
			if (p5==0){
				processNumber = 7;
			}
		}
	}

	if ((h=gethostbyname(HOST)) == NULL) {  // get the host info
		herror("gethostbyname");
		exit(1);
	}

	int portN;
	//assigns port number
	if (processNumber == 1) portN = UDP1;
	if (processNumber == 2) portN = UDP2;
	if (processNumber == 3) portN = UDP3;
	if (processNumber == 4) portN = UDP4;
	if (processNumber == 5) portN = UDP5;
	if (processNumber == 6) portN = UDP6;
	if (processNumber == 7) portN = UDP7;
	printf("Rover %d has UDP port %d and IP address \"%s\"\n", processNumber, portN, inet_ntoa(*((struct in_addr *)h->h_addr)));

	//PORT NUMBER = 21100 + 132 (last three digits of ID) + processNumber * 100
	//int portNum = 21100 + 132 + processNumber * 100;
	FILE *fp;

	if ((fp = fopen("scenario.txt", "r")) == NULL) {
		printf("Couldn't open scenario.txt for reading\n");
		exit(1);
	}
	fpos_t pos;
	fgetpos(fp, &pos);   // save the position at the beginning of the file

	//read data from file
	getData(0, fp, &antenna1, &pos);
	getData(1, fp, &antenna2, &pos);
	getData((processNumber+1),fp, &own, &pos);

	//calculate the Cartesian distance to each of the antennas
	distToA1 = calculateDistance(&antenna1);
	distToA2 = calculateDistance(&antenna2);
	totalDist = distToA1 + distToA2;
	//here the rovers are exchanging HELO type packets
	switch (processNumber){
	case 1:
		sleep(1); sendHelo(UDP2); sendHelo(UDP3); sendHelo(UDP4); sendHelo(UDP5);			
		sendHelo(UDP6);	sendHelo(UDP7);	receiveHelo(UDP1);	receiveHelo(UDP1);receiveHelo(UDP1);			
		receiveHelo(UDP1);	receiveHelo(UDP1);receiveHelo(UDP1);
		break;
	case 2: 	
		receiveHelo(UDP2);sleep(1);
		sendHelo(UDP1);	sendHelo(UDP3);	sendHelo(UDP4);	sendHelo(UDP5);	sendHelo(UDP6);		
		sendHelo(UDP7);	receiveHelo(UDP2);	receiveHelo(UDP2);	receiveHelo(UDP2);	receiveHelo(UDP2);
		receiveHelo(UDP2);	
		break;
	case 3: 
		receiveHelo(UDP3);	receiveHelo(UDP3);	sleep(1);
		sendHelo(UDP1);	sendHelo(UDP2);	sendHelo(UDP4);	sendHelo(UDP5);		
		sendHelo(UDP6);	sendHelo(UDP7);	receiveHelo(UDP3);	receiveHelo(UDP3);
		receiveHelo(UDP3);	receiveHelo(UDP3);
		break;
	case 4: 
		receiveHelo(UDP4);	receiveHelo(UDP4);	receiveHelo(UDP4);	sleep(1);
		sendHelo(UDP1);	sendHelo(UDP2);	sendHelo(UDP3);	sendHelo(UDP5);	
		sendHelo(UDP6);	sendHelo(UDP7);	receiveHelo(UDP4);	receiveHelo(UDP4);
		receiveHelo(UDP4);	break;
	case 5: 
		receiveHelo(UDP5);	receiveHelo(UDP5);	receiveHelo(UDP5);	receiveHelo(UDP5);
		sleep(1); sendHelo(UDP1); sendHelo(UDP2);	sendHelo(UDP3);	sendHelo(UDP4);
		sendHelo(UDP6);	sendHelo(UDP7);	receiveHelo(UDP5);	receiveHelo(UDP5);
		break;
	case 6: 
		receiveHelo(UDP6);receiveHelo(UDP6);receiveHelo(UDP6);receiveHelo(UDP6);receiveHelo(UDP6);
		sleep(1);	sendHelo(UDP1);	sendHelo(UDP2);	sendHelo(UDP3);	sendHelo(UDP4);	sendHelo(UDP5);		
		sendHelo(UDP7);	receiveHelo(UDP6);	
		break;
	case 7: 
		receiveHelo(UDP7);	receiveHelo(UDP7);	receiveHelo(UDP7);	receiveHelo(UDP7);			
		receiveHelo(UDP7);	receiveHelo(UDP7);	sleep(1);
		sendHelo(UDP1);	sendHelo(UDP2);	sendHelo(UDP3);	sendHelo(UDP4);	sendHelo(UDP5);		
		sendHelo(UDP6);	
		break;
	}

	if (compareDist()) {//rendevouz Point

		RP = processNumber;
		sleep(1);
		//here RP sends ImRP packet to every rover that is not RP
		for (int k =1; k<8; k++){
			if (k!=processNumber){
				sendPacket((21132 +k*100), " ImRP ", " I'mRandPoint ");
			}
		}
		//printf("Rover %d: Done sending ImRP packets.\n", processNumber);
		printf("END OF PHASE 1\n");


//here RP receives subscription packets from other rovers
		receivePacket((21132 +RP*100), &a[1], &c[1] );
		receivePacket((21132 +RP*100), &a[2], &c[2] );
		receivePacket((21132 +RP*100), &a[3], &c[3] );
		receivePacket((21132 +RP*100), &a[4], &c[4] );
		receivePacket((21132 +RP*100), &a[5], &c[5] );
		receivePacket((21132 +RP*100), &a[6], &c[6] );
		//here RP processes packets and detemines the antenna each rover subscribed to
		int aNumSubscribed [8];
		for (int k = 1; k<7; k++){		
			aNumSubscribed[a[k]] = atoi(c[k]);		
		}
		
		//printf("Rover %d: Done Receiving Join packets\n", processNumber);
		sleep(1);
		//taken drom Beej Guide
		if ((he=gethostbyname(HOST)) == NULL) {  // get the host info 
			herror("gethostbyname");
			exit(1);
		}

		if ((sockfd3 = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			exit(1);
		}

		if ((sockfd4 = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
			perror("socket");
			exit(1);
		}

		myAddr.sin_family = AF_INET;
		myAddr.sin_port = 0;
		myAddr.sin_addr.s_addr = INADDR_ANY;

		thaddr1.sin_family = AF_INET;    // host byte order 
		thaddr1.sin_port = htons(TCP1);  // short, network byte order 
		thaddr1.sin_addr = *((struct in_addr *)he->h_addr);
		memset(thaddr1.sin_zero, '\0', sizeof thaddr1.sin_zero);

		thaddr2.sin_family = AF_INET;    // host byte order 
		thaddr2.sin_port = htons(TCP2);  // short, network byte order 
		thaddr2.sin_addr = *((struct in_addr *)he->h_addr);
		memset(thaddr2.sin_zero, '\0', sizeof thaddr2.sin_zero); 		

		char bufer1[MAXBUFLEN], bufer2[MAXBUFLEN] ;
//building DATA packet that is send to antenna
		char *flag1 = " Data ";
		char m1[5], m2[5];
		deltax2 = own.x - antenna2.x; deltax1 = own.x - antenna1.x;
		deltay2 = own.y - antenna2.y; deltay1 = own.y - antenna1.y;
		int i1, i2;
		i1 = i2 = 0;
		//determining which number is negative and how many minuses should be there
		if (deltax2 < 0) { deltax2 = abs(deltax2); i2 = 1; }
		if (deltax1 < 0) { deltax1 = abs(deltax1); i1 = 1; }
		if (deltay2 < 0) { deltay2 = abs(deltay2); if (i2==1) i2=3; else i2=2;}
		if (deltay1 < 0) { deltay1 = abs(deltay1); if (i1==1) i1=3; else i1=2;}

		sprintf(dx2, "%d", deltax2 );
		sprintf(dy2, "%d", deltay2 );				
		sprintf(dx1, "%d", deltax1 );
		sprintf(dy1, "%d", deltay1 );
		sprintf(m1, "%d", i1 );
		sprintf(m2, "%d", i2 );
		//printf("dx2 = %s dx1 = %s dy2 = %s dy1 = %s\n deltax2 = %d", dx2, dx1, dy2, dy1, deltax2);
		char frame1[30];
		char frame2[30];
		strcpy(frame1, "");
		strcpy(frame2,"");
		char * h1 = "rover ";
		char roverNum[5];
		sprintf(roverNum, "%d", processNumber);
		strcat(frame1, h1); 
		strcat(frame1, roverNum);
		strcat(frame1, flag1);
		strcat (frame1, m1);
		strcat (frame1, " ");
		strcat(frame1, dx1);
		strcat(frame1, " ");
		strcat(frame1, dy1);
		strcat(frame2, h1);
		strcat(frame2, roverNum);
		strcat(frame2, flag1);
		strcat (frame2, m2);
		strcat(frame2, " ");
		strcat(frame2, dx2);
		strcat(frame2, " ");
		strcat(frame2, dy2);
		sleep(2);

		//printf("Rover %d connecting to A1\n", processNumber);
		if (connect(sockfd3, (struct sockaddr *)&thaddr1,sizeof thaddr1) == -1) {
			perror("connect");
			exit(1);
		}

		//printf("Rover %d Connecting to A2\n", processNumber);
		if (connect(sockfd4, (struct sockaddr *)&thaddr2,sizeof thaddr2) == -1) {
			perror("connect");
			exit(1);
		}

		socklen_t namesock1, namesock2;
		namesock2 = sizeof(myAddr);
		namesock1 = sizeof(myAddr);
		//determining the dynamic port number
		if (getsockname(sockfd4, (struct sockaddr *) &myAddr, &namesock2))
		{ perror("get port number");
		exit(-1); }
		int tcpPort2 = (int) ntohs(myAddr.sin_port);
		if (getsockname(sockfd3, (struct sockaddr *) &myAddr, &namesock1))
		{ perror("get port number");
		exit(-1); }
		int tcpPort1 = (int) ntohs(myAddr.sin_port);
		printf("Rover %d has TCP ports %d and %d\n", processNumber, tcpPort1, tcpPort2);
//sending the data packets to antennas
		if (send(sockfd3, frame1, strlen(frame1), 0) == -1){
			perror("send");
			exit(1);
		}
		if (send(sockfd4, frame2, strlen(frame2), 0) == -1){
			perror("send");
			exit(1);
		}
//receiving antenna packets with its own coordinates
		if ((numb2=recv(sockfd4, bufer2, 99, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		bufer2[numb2] = '\0';
		//printf("Rover %d Received: %s\n",processNumber, bufer2);
		
		if ((numb1=recv(sockfd3, bufer1, 99, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		bufer1[numb1] = '\0';
		//printf("Rover %d Received: %s\n",processNumber, bufer1);
//building REQUEST FOR CONTENT packet
		char space[] = " ";
		char *r = strtok( bufer2, space );
		char *r1 = strtok(NULL, space);  	
		int aN = atoi(r1);
		char *type = strtok(NULL, space);
		char *xrec = strtok( NULL, space );
		int xr = atoi(xrec);
		char *yrec = strtok( NULL, space );
		int yr = atoi(yrec);
		strcpy(frame2,"");
		strcat(frame2, h1);
		strcat(frame2, roverNum);
		strcat(frame2, flag1);
		strcat (frame2, " RequestForContent ");
//if the antenna's packet did indeed contain its own coordinates then rover sends the packet request for content
		if (xr==own.x&& yr ==own.y){
			if (send(sockfd4, frame2, strlen(frame2), 0) == -1){
				perror("send");
				exit(1);
			}  		
		}

		r = strtok( bufer1, space );
		r1 = strtok(NULL, space);  	
		aN = atoi(r1);
		type = strtok(NULL, space);
		xrec = strtok( NULL, space );
		xr = atoi(xrec);
		yrec = strtok( NULL, space );
		yr = atoi(yrec);
		if (xr==own.x&& yr ==own.y){
			//same done for the other antenna
			if (send(sockfd3, frame2, strlen(frame2), 0) == -1){
				perror("send");
				exit(1);
			}  		
		}
		//receiving A2broadcasting packet
		if ((numb2=recv(sockfd4, bufer2, 99, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		bufer2[numb2] = '\0';
		strtok( bufer2, space );
		strtok(NULL, space);  	
		strtok(NULL, space);
		char *lastp2 =  strtok( NULL, space );
		//printf("Rover %d Received: %s\n",processNumber, bufer2);
		
		//receiving A1broadcasting packet
		if ((numb1=recv(sockfd3, bufer1, 99, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		bufer1[numb1] = '\0';
		strtok( bufer1, space );
		strtok(NULL, space);  	
		strtok(NULL, space);
		char *lastp1 =  strtok( NULL, space );
	//	printf("Rover %d Received: %s\n",processNumber, bufer1);
		sleep(1);
		
		//sending the A2broadcasting or A1 broadcasting to rovers determined on their subscription
		for (int k= 1; k<8; k++){
			if (k!=processNumber){
			//printf(" k = %d, aNumSubscribed = %d \n", k, aNumSubscribed[k]);
			if (aNumSubscribed[k]==1){
					sendPacket(21132+ k*100, " Data ", lastp1);
					}
			if (aNumSubscribed[k]==2){
					sendPacket(21132+ k*100, " Data ", lastp2);
				}
			}
		}
		close(sockfd3);
		close(sockfd4);
	}
	else {//not RP
		int rN;
		char * c;
		//receiving IMRP type packet from RP
		receivePacket((21132 + processNumber*100),&rN, &c);
		RP = rN;
		char *dest;
		if (distToA1<= distToA2)
			dest = "antenna 1 ";
		else 
			dest = "antenna 2 ";
		switch (processNumber){
		case 1: sleep(1); break;
		case 2: sleep (2); break;
		case 3: sleep(3); break;
		case 4: sleep(4); break;
		case 5: sleep(5); break;
		case 6: sleep(6); break;
		case 7: sleep(7); break;
		}
		//printf("Rover %d: sending Join packet to Rendezvous point rover %d.\n", processNumber, RP);
		printf("Rover %d subscribed to %s\n", processNumber, dest);
		sleep(1);
		sendPacket((21132 + RP*100), " Join ", dest);
		int nn;
		char *s1;
		receivePacket((21132 +processNumber*100), &nn, &s1 );
	}
	printf ("End of phase 3 for rover %d\n", processNumber);
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
	//printf( "item is %s X is %s Y is %s proc Num is %d\n",result1, result2, result3, processNumber );
}
int calculateDistance(struct coord *c){
	//printf("x-x%d y-y %d", (own.x-c->x), (own.y - c->y));
	float dist = sqrt(pow((own.x-c->x),2)+pow((own.y-c->y),2));
	int result = (int)ceil(dist);
	if (result > 50) result = 50;
	//printf("process %d distance = %d\n",processNumber, result);
	return result;
}
void sendHelo(int port){
//taken from Beeej guide
	if ((h=gethostbyname(HOST)) == NULL) {  // get the host info
		herror("gethostbyname");
		exit(1);
	}
	int socketDescr;
	if ((socketDescr = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	theirAddress.sin_family = AF_INET;     // host byte order
	theirAddress.sin_port = htons(port); // short, network byte order
	theirAddress.sin_addr = *((struct in_addr *)h->h_addr);
	memset(theirAddress.sin_zero, '\0', sizeof theirAddress.sin_zero);
	char frame [30];
	char * h1 = "rover ";
	char roverNum[5];
	sprintf(roverNum, "%d", processNumber);
	char *flag=" Helo ";
	char num[5];
	sprintf(num, "%d", totalDist );
	strcat(frame, h1);
	strcat(frame, roverNum);
	strcat(frame, flag);
	strcat(frame, num);
	//printf("packet = %s\n", frame);
	if ((numbytes = sendto(socketDescr, frame, strlen(frame), 0,
			(struct sockaddr *)&theirAddress, sizeof theirAddress)) == -1) {
		perror("sendto");
		exit(1);
	}

	//printf("rover %d sent packet *%s* send num %d to port %d\n",processNumber, frame, isend, port);
	frame[0] = '\0';
	//printf("sent %d bytes to %s\n", numbytes, inet_ntoa(theirAddress.sin_addr));

	close(socketDescr);
}
void receiveHelo(int port){
	//taken from beej guide
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_port = htons(port);
	sockAddress.sin_addr.s_addr = INADDR_ANY;
	memset(sockAddress.sin_zero, '\0', sizeof sockAddress.sin_zero);

	

	int sockfd2;
	if ((sockfd2 = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (bind(sockfd2, (struct sockaddr *)&sockAddress, sizeof (sockAddress))==-1){
		printf("Couldn't bind a socket to a port number\n");
		exit(1);
	}
	
	struct sockaddr_in fromAddr;
	addr_len = sizeof (fromAddr);
	if ((numbytes = recvfrom(sockfd2, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&fromAddr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	//printf("got packet from %s\n",inet_ntoa(fromAddr.sin_addr));
	//printf("packet is %d bytes long\n",numbytes);
	buf[numbytes] = '\0';
	//printf("rover %d got packet \"%s\"\n", processNumber, buf);
	int len = strlen(buf);
	char space[] = " ";
	char *r = strtok( buf, space );
	char *r1 = strtok(NULL, space);
	int ind = atoi(r1);
	char *r2 = strtok(NULL, space);
	char *r3 = strtok( NULL, space );
	receivedDist[ind] = atoi(r3);
	//printf ("rover %d received dist %d from rover %d\n", processNumber, receivedDist[ind], ind, );
	close(sockfd2);


}
void displayDist(){ //for debugging
	for (int k =1; k<8; k++){
		printf("%d: dist[%d] = %d \n",processNumber,k, receivedDist[k]);
	}
}
bool compareDist(){
	//determines here who is RP
	bool min = 0;

	receivedDist[processNumber] = totalDist;
	int minR= 1;
	int minD = receivedDist[1];
	for (int k = 1; k <8; k++){
		if ((receivedDist[k]<minD)) {minR = k; minD = receivedDist[k];} 
	}
	if (minR==processNumber) min = 1;
	return min;
}
void sendPacket(int port, char *type, char *payload){
	//taken from Beej guide
	if ((h=gethostbyname(HOST)) == NULL) {  // get the host info
		herror("gethostbyname");
		exit(1);
	}
	int socketDescr;
	if ((socketDescr = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	theirAddress.sin_family = AF_INET;     // host byte order
	theirAddress.sin_port = htons(port); // short, network byte order
	theirAddress.sin_addr = *((struct in_addr *)h->h_addr);
	memset(theirAddress.sin_zero, '\0', sizeof theirAddress.sin_zero);
	char frame [30];
	char * h1 = "rover ";
	char roverNum[5];
	sprintf(roverNum, "%d", processNumber);
	char *flag=type;
	strcat(frame, h1);
	strcat(frame, roverNum);
	strcat(frame, flag);
	strcat(frame, payload);
	//printf("packet = %s\n", frame);
	if ((numbytes = sendto(socketDescr, frame, strlen(frame), 0,
			(struct sockaddr *)&theirAddress, sizeof theirAddress)) == -1) {
		perror("sendto");
		exit(1);
	}
	printf("");
	frame[0] = '\0';
	//printf("sent %d bytes to %s\n", numbytes, inet_ntoa(theirAddress.sin_addr));
	close(socketDescr);
}
void receivePacket(int port, int *rN, char **str){
//taken from Beej guide
	sockAddress.sin_family = AF_INET;
	sockAddress.sin_port = htons(port);
	sockAddress.sin_addr.s_addr = INADDR_ANY;
	memset(sockAddress.sin_zero, '\0', sizeof sockAddress.sin_zero);
	int sockfd2;
	if ((sockfd2 = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
	if (bind(sockfd2, (struct sockaddr *)&sockAddress, sizeof (sockAddress))==-1){
		printf("Couldn't bind a socket to a port number\n");
		exit(1);
	}
	struct sockaddr_in fromAddr;
	addr_len = sizeof (fromAddr);
	if ((numbytes = recvfrom(sockfd2, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&fromAddr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
	buf[numbytes] = '\0';	
	int len = strlen(buf);
	char space[] = " ";
	char *r = strtok( buf, space );
	char *r1 = strtok(NULL, space);
	*rN = atoi(r1);
	char *type = strtok(NULL, space);
	char *payload = strtok( NULL, space );
	char *an;
	if( strcmp( payload, "antenna" ) == 0 ){
		an = strtok(NULL, space);
	}
	char p[20];
	strcpy(p, payload);
	if (strlen(an)!=0){
		strcpy(p, an);
	}

	*str = (char *) malloc((strlen(p)) * sizeof(char)); 
	strcpy((char *)*str, p);
	
	//printf ("rover %d received  %s, type %s from rover %d\n", processNumber, payload, type, *rN );
	
	printf("");

	close(sockfd2);

}
