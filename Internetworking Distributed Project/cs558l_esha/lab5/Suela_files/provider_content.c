/*
 * File provider_content.c
 * Author:Suela Buzi 
 * Description: UDP hardcoded port,  * same ip as the server 
 * registers will bootstrap_server and receives peer info through directory.txt
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
#define UDP1 4845
#define UDP2 4945

#define TEMP_BUF_SIZE 100

int portID;
int providerID=0;
char OutBuf1[TEMP_BUF_SIZE];
char InBuf1[TEMP_BUF_SIZE];


int main(int argc, char *argv[])
{
	int UDP_sockfd;
	int TCP_sockfd;

	struct sockaddr_in my_UDP_addr; 
	struct sockaddr_in their_addr; 

	struct hostent *he;
	int numbytes;

	socklen_t addr_len;

	addr_len = sizeof(struct sockaddr);

	/*
	 * Get the host by name 
	 * hardcoded to nunki 
	 */
	if ((he=gethostbyname("nunki.usc.edu")) == NULL) {
		perror("Unable to gethostbyname");
		exit(1);
	}
	/*
	 * Fork creates two processes for two providers 
	 */
	if( fork() == 0 )
		providerID++;

	if (providerID ==0)portID=UDP1;
	if (providerID ==1)portID=UDP2;

	/*
	 * Openign UDP socket 
	 */
	if ((UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("UDPsocket");
		exit(1);
	}
	
	/*
	 * Binding UDP socket 
	 */
	
	my_UDP_addr.sin_family = AF_INET;
	my_UDP_addr.sin_port = htons (portID); 
	my_UDP_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_UDP_addr.sin_zero), '\0', 8);

	if (bind(UDP_sockfd, (struct sockaddr *)&my_UDP_addr,
			sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}
	
	/*
	 * Sending registration data 
	 */
	
	sprintf(OutBuf1, "%s", "provider");
	char prn[1];
	sprintf(prn, "%d",providerID+1 );
	strcat(OutBuf1, prn);
	strcat(OutBuf1," provider spring07");

	their_addr.sin_family = AF_INET; // host byte order
	their_addr.sin_port = htons(SERVERPORT); // short, network byte order
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8); // zero the rest of the struct
	

	if ((numbytes = sendto(UDP_sockfd, OutBuf1, strlen(OutBuf1), 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
		perror("Error sending data to bootserver");
		exit(1);
	}

	//printf("sending registration data/n"); // debug 
	//printf ("%s",OutBuf1); // debug 

	close(UDP_sockfd);
	return 0;
}
