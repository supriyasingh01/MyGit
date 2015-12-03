 Instructions to run the code:

1. Start zebra on rtr1 and rtr2 and change the default gateway for all the nodes to their respective routers
2. Copy the configuration file router.conf and the router.c file into one directory on usRTR. 
			-> router.conf contains the configuration file..1st address:N/w Address, 2nd address: Next HOp Address, 3rd Address: subnet Mask 4th:interface
3. Edit the router.conf file. Change the interfaces (i.e eth6,eth8) to the interfaces on usRTR used to reach the respective networks

4. Enter following command to compile: >> gcc -o router router.c -lpthread -lpcap -lnsl -lresolve
5. Enter following command to run: >> sudo ./router router.conf
6. Ping node5/node6 from node0 or anyother way.