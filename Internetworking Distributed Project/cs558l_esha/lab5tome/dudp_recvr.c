/*
RECEIVER
*/
#include "headers.h"

int main(void)
{
    int sockfd[MAXSOCKETS];
    int sockfd1[MAXSOCKETS];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    //char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    int i;
    struct message m2;
    pthread_t thread[NUM_THREADS];
    FILE* out_file;
	out_file = fopen ("outputfile.out","a");
	int socketCount=0;
	int threadCount=0;
	int tid;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { //PORT= myPort
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd[socketCount] = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("RECEIVER: socket");
            continue;
        }

        if (bind(sockfd[socketCount], p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd[socketCount]);
            perror("RECEIVER: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "RECEIVER: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("RECEIVER: waiting to recvfrom...\n");
    
    // handle data from a sender
	
    addr_len = sizeof their_addr;
    while(1)
	{	
		if ((numbytes = recvfrom(sockfd[0], &m2, sizeof(struct message), 0,(struct sockaddr *)&their_addr, &addr_len)) <=0) 
			{
				// got error or connection closed by sender
				if (numbytes == 0)
				{
					// connection closed
					printf("sender socket hung up\n");
				} 
				else 
				{
					perror("recvfrom");
				}
			}
		else
		{
			printf("filesize is:%d\n",m2.fileSize);
			printf("data type is:%d\n",m2.data_type);
			
			switch(m2.data_type) {
			case 1: ;
					printf("It's a Connection Request!\n");
					//make a new thread to handle data from this new connection here......
					socketCount++;
					threadCount++;
					tid = pthread_create(&thread[threadCount], NULL, makeNewSocket ,(void*)&socketCount );
					break;
					
			case 2: ;
					//find its respective thread and socket*********************
					if ((numbytes = recvfrom(sockfd1[socketCount+1], &m2, sizeof(struct message), 0,(struct sockaddr *)&their_addr, &addr_len)) <=0) 
					{
						// got error or connection closed by sender
						if (numbytes == 0)
						{
							// connection closed
							printf("sender socket hung up\n");
						} 
						else 
						{
							perror("recvfrom");
						}
					}
					else
					{
						printf("Data in file is:");
					
						for(i=0;i<m2.fileSize;i++)
						{
							
							printf("%c",m2.data[i]);
							putc(m2.data[i],out_file);
							
						}
					
						fclose (out_file);
						break;
					}
			}//end of switch cases
		}//end of else
	}//end of while loop
	
	
    printf("RECEIVER: got packet from %s\n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
    //printf("RECEIVER: packet is %d bytes long\n", numbytes);
   

    close(sockfd[0]);

    return 0;
}
