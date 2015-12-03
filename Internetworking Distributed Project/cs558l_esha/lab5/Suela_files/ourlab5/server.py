#!/usr/bin/python
from socket import *
from sys import argv
from thread import start_new_thread
from threads import *
from packetize import *
import copy


sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(('',int(argv[1])))
print "Server waiting for connection"

while 1: # Run until cancelled
	filename, client_addr = sock.recvfrom(256)
	packet_List  = packets(filename, 1500, 1400) #sends to function to break up and return list of packets
	packet_List1 = copy.copy(packet_List)
	packet_List2  = copy.copy(packet_List)
	packet_List3  = copy.copy(packet_List)
	packet_List4  = copy.copy(packet_List)
	
	#for i in range (0,5):
	start_new_thread(send_file, (sock, filename, client_addr,packet_List))
	start_new_thread(send_file, (sock, filename, client_addr,packet_List1))
	start_new_thread(send_file, (sock, filename, client_addr,packet_List2))
	start_new_thread(send_file, (sock, filename, client_addr,packet_List3))
	start_new_thread(send_file, (sock, filename, client_addr,packet_List4))
		
