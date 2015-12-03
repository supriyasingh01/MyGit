/*
 * File peer.c
 * Author:Suela Buzi 
 * Description: UDP hardcoded port,  * same ip as the server 
 * sends information about peer port id and group chosen to server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERVERPORT 3745 
#define TEMP_BUF_SIZE 100


char OutBuf1[TEMP_BUF_SIZE];
char OutBuf2[TEMP_BUF_SIZE];
int peerNum;


#define UDP1  5845
#define UDP2  5945
#define UDP3  6045
#define UDP4  6145

int main(int argc, char *argv[])
{
	int UDP_sockfd;
	int TCP_sockfd;
	//	int new_fd;

	struct sockaddr_in my_UDP_addr; 
	struct sockaddr_in my_TCP_addr; 
	struct sockaddr_in their_addr; 

	int len = sizeof(struct sockaddr_in);;
	struct hostent *host;
	int numbytes;
	socklen_t sin_size;
	int portN;
/*
 * Host found by gethostbyname - hardcoded to nunki
 */
	if ((host=gethostbyname("nunki.usc.edu")) == NULL) {
		perror("Unable to gethostbyname");
		exit(1);
	}
	peerNum=1;
	while(peerNum<4){
			if( fork() == 0 )
				peerNum++;
			else
				break;
		}


/*
 * Port numbers are hardcoded and will be assigned depending on the
 * peer# and the order of  the fork() activation 
 */
	if (peerNum == 0) portN = UDP1;
	if (peerNum == 1) portN = UDP2;
	if (peerNum == 2) portN = UDP3;
	if (peerNum == 3) portN = UDP4;

	
	/*
	 * Creating and binding TCP socket to the dynamically allocated ports
	 * this ports will be used to communicated with the providers 
	 */
	if ((TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("TCPsocket");
		exit(1);
	}
	my_TCP_addr.sin_family = AF_INET; 
	my_TCP_addr.sin_port = INADDR_ANY;
	my_TCP_addr.sin_addr = *((struct in_addr *)host->h_addr);

	memset(&(my_TCP_addr.sin_zero), '\0', 8); 

	if (bind(TCP_sockfd, (struct sockaddr *)&my_TCP_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("error binding tcp socket");
		exit(1);
	}

	if(getsockname(TCP_sockfd,(struct sockaddr *)&my_TCP_addr,&len )<0){
		perror("error TCP getsockname");
		exit(1);
	}
/*
 * Opening UDP socket 
 */
	if ((UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Unable to open UDPsocket");
		exit(1);
	}
/*
 * Binding  UDP socket 
 */
	my_UDP_addr.sin_family = AF_INET; 
	my_UDP_addr.sin_port = htons(portN); 
	my_UDP_addr.sin_addr.s_addr = INADDR_ANY; 
	memset(&(my_UDP_addr.sin_zero), '\0', 8); 
	if (bind(UDP_sockfd, (struct sockaddr *)&my_UDP_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("error binding UDP socket");
		exit(1);
	}

/*
 * Sending registration data to server 
 * peer[i] ip port# group#
 */
	
	sprintf(OutBuf1, "%s","peer"); 
	char pn[1]; // /temporary storage OutBuf1 will not accept overwriting
	sprintf(pn,"%d",peerNum);
	strcat(OutBuf1, pn);
	strcat(OutBuf1," ");
	sprintf(OutBuf2, "%d",my_TCP_addr.sin_port); // Port#
	OutBuf2[5]=' ';
	strcat(OutBuf1, OutBuf2);
	strcat(OutBuf1," ");
	strcat(OutBuf1,inet_ntoa(my_TCP_addr.sin_addr)); //IP Address

	if(peerNum==1|| peerNum==3)
		strcat(OutBuf1," group1"); 
	/*
	 * compiler on nunki was unable to handle random generation 
	 * of the group value 
	 * Contacted the IT of USC they walked me through a long process
	 * and it did not work, they said it had something to do will 
	 * my account - so i generate group 1 form peer1 and peer3
	 * and group2 for peer2 and peer4
	 */
	else
		strcat(OutBuf1," group2");


	their_addr.sin_family = AF_INET; // host byte order
	their_addr.sin_port = htons(SERVERPORT); // short, network byte order
	their_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8); // zero the rest of the struct
	/*send the registration data to boot strap server*/
	//printf ("check peer send info \n"); // debug 

	printf("%s", OutBuf1);
	printf ("\n");

	if ((numbytes = sendto(UDP_sockfd, OutBuf1, strlen(OutBuf1), 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
		perror("sendto");
		exit(1);
	}
	close(UDP_sockfd);

	/////
	/// content provider
	////
	exit(1);
}

