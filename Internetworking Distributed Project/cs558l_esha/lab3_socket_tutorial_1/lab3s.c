/*

** selectserver.c 

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
//#define PORT "5501"   // port we're listening on
#define MAXBUFLEN 255

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char* argv[])

{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    socklen_t addr_len;
	struct sigaction sa; 
 // struct sockaddr_storage their_addr;
    char bufr[MAXBUFLEN];    // buffer for receiving
    char bufs[MAXBUFLEN];	//buffer for sending
    int nbytes,numbytes,recbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    char s[INET6_ADDRSTRLEN];
    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i,l, j, rv,count;
    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &ai)) != 0) {

        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {

        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (listener < 0) { 
            continue;
			}

			// lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;

    }
    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
	
	sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
	
    printf("\nServer waiting for client......\n");
	
	// add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // main loop
    for(;;) 
	{

        read_fds = master; // copy it

        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
         // run through the existing connections looking for data to read

        for(i = 0; i <= fdmax; i++)
		{
            if (FD_ISSET(i, &read_fds)) 
			{ // we got one!!
                if (i == listener) 
				{
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,(struct sockaddr *)&remoteaddr,&addrlen);

                    if (newfd == -1) 
					{
                        perror("accept");
                    } 

					else 
					{

                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("\nServer: new connection from %s on socket %d\n",

                        inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN),newfd);

                    }
                } 
				else 
				{
                    // handle data from a client

					printf("server: waiting to recv ...\n");
					//RECEIVE ACTUAL DATA
				
					if ((nbytes = recv(i, bufr, sizeof (bufr), 0)) <= 0) 
					{
						// got error or connection closed by client
						if (nbytes == 0)
						{
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						} 

						else 
						{
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set

					} 

					else 
					{
						printf("\nserver: receiving data from Client\n");
						printf("Got packet from client\n");
						printf("data is:");
						for(i=0;i<MAXBUFLEN;i++)
							{
								printf("%c",bufr[i]);
							}
							printf("\n\n");

					}		
                } // END handle data from client

            } // END got new incoming connection

        } // END looping through file descriptors

    } // END for(;;)--and you thought it would never end!

    return 0;

}

//lab3sm
//
