c. In this assignment I have created 7 processes that communicate with each other to determine which one should be the RP for communicating with other two processes - the antennas. Antennas and RP rover communicates through TCP sockets, while the rovers communicate through UDP sockets. 

d. rover.c - uses fork() to create 7 processes, sends and receive UDP(or TCP) packets; antenna.c - uses fork() to create 2 processes, communication done through TCP packets

e. please compile the files with these commands : 
	g++ -o rover rover.c -lsocket -lnsl -lresolv
	g++ -o antenna antenna.c -lsocket -lnsl -lresolv

Please run the program by typing "antenna & rover"

Also, after running the program, please wait 20 sec or so before running it again. Even if you look for any left over processes, there won't be any, but for some reason, the program may not run correctly if it is ran immediately after the previous run.

