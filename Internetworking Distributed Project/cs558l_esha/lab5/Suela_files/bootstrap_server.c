/*
 * File Bootstrap Server 
 * Author:Suela Buzi 
 * Description: Server IP hardcoded as nunki.usc.edu, bind on the 3745 port number 
 * receives from four peer and two content providers 
 * creates "directory.txt" and writes the incomming message from the peers 
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

#define MAX_BUF_SIZE 100
#define SERVERPORT 3745


	int main()
	{
		int sock; //sock id

		int addr_len;
		int numbytes;
		char buff_in[MAX_BUF_SIZE];

		struct sockaddr_in my_addr; 
		struct sockaddr_in their_addr; 
		
		//  reused code form Beej's guide


		FILE *directory;
		/*
		 * creting directory file 
		 */
		directory = fopen("directory.txt", "w");
		
		/*
		 * opening UDP socket 
		 * this socket will be bound to the port to receive incomming messages from 
		 * peers and content providers 
		 */

		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("Unable to open socket ");
			exit(1);
		}

		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(SERVERPORT);
		my_addr.sin_addr.s_addr = INADDR_ANY;
		memset(&(my_addr.sin_zero), '\0',8);

		/*
		 * binding UDP_SOCKET 
		 */
		if (bind(sock,(struct sockaddr *)&my_addr,
				sizeof(struct sockaddr)) == -1)
		{
			perror("Unable to bind UDP socket to the assigned port");
			exit(1);
		}
		else
			//printf("Binding server to port successful"); // debug comment 

		addr_len = sizeof(struct sockaddr);

		int peer_in =0;

		/*waiting for all peers to register*/

		for(peer_in=1;peer_in<5;peer_in++){
			if ((numbytes = recvfrom(sock, buff_in, MAX_BUF_SIZE-1 ,
					0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
				perror("Error receiving from the peers \n");
				exit(1);
			}

			buff_in[numbytes] = '\0'; ///end line to separate users

			/*Writing peer info to "directory.txt"*/
			fprintf(directory,"%s\n",buff_in); // /write buffer content into the file
			char p = (char) peer_in;

			if (buff_in[0]=='p' && buff_in[1]=='e' && buff_in[2]=='e' && buff_in[3]=='r')
				printf("Peer%d registration is done successfully!\n",peer_in);
			else
				perror("Unable to register peers");

		}
		printf("Registration of peers completed!\n");
		printf("Run the content_providers!\n");

		fclose(directory);

		int provider_in =0;
		/*
		 * Provider registration 
		 */
		for (provider_in=1; provider_in<3; provider_in++){
			if ((numbytes = recvfrom(sock, buff_in, MAX_BUF_SIZE-1 ,
					0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
				perror("Error receiving from providers \n");
				exit(1);
			}

			buff_in[numbytes] = '\0'; /// end line
			//			printf("%s", buff_in);
			printf("Incoming message from content_provider%d\n",provider_in);
			/*
			 * Verifying registration of the providers
			 * username: provider 
			 * password: spring07
			 */
			if (buff_in[0]=='p' && buff_in[1]=='r' && buff_in[2]=='o' && buff_in[3]=='v'&& buff_in[4]=='i'&&
					buff_in[5]=='d' && buff_in[6]=='e' && buff_in[7]=='r')

				/*	if ((buff_in[0]=='p' && buff_in[1]=='r' && buff_in[2]=='o' && buff_in[3]=='v'&& buff_in[4]=='i'&&
				buff_in[5]=='d' && buff_in[6]=='e' && buff_in[7]=='r')
				&&
				(buff_in[9]=='s'&&buff_in[10]=='p'&&buff_in[11]=='r'&&buff_in[12]=='i'&&buff_in[13]=='n'
				&&buff_in[14]=='g'&& buff_in[15]=='0'&&buff_in[16]=='7'))*/

				printf("Provider%d registration is done successfully!\n",provider_in);
			else
				printf("Provider registration unsuccessfully!\n");

		}
		printf("Registration of providers completed!\n");

		close(sock);
		return 0;
	}
