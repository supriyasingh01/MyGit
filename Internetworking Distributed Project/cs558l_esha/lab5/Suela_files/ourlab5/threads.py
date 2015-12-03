#!/usr/bin/python
from socket import *
import sys, time
from sys import argv
from thread import start_new_thread, get_ident
from packetize import *
from time import localtime
def send_file(sock, filename, client_addr,packet_List):    #called by server
       	from time import sleep
	print "Client connected:", client_addr
	file = open(filename, "rb")
	
	print "starting to send...."
	print "1",localtime()

	while(len(packet_List)!= 0):
		packet = packet_List.pop(0)
		#print packet
		sock.sendto(str(packet), client_addr)
	
	
	print "Finished Sending File  "
	print "2",localtime()
def request(sock,threads):    #called by client
	print sys.argv[2]
	sock.sendto("%s" % (sys.argv[2]),(sys.argv[1], int(sys.argv[3])))

	#messin, server = sock.recvfrom(255)
	#num_of_packets = messin[2]
	packets_recieved = 1
	#print "Total Number of packets is ", num_of_packets
	
	
        
	for i in range(0, 149796):
		messin, server = sock.recvfrom(255)
        	packets_recieved += 1
		print "Received:", messin

	print "recvd last" 
	print time.localtime()
	del threads[get_ident()]
