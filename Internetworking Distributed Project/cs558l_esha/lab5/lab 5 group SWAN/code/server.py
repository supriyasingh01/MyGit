#!/usr/bin/python
from socket import *
from sys import argv
import os
import sys, time, string
from time import sleep
import time
import math




sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(('',int(argv[1])))


port_num= int(sys.argv[1])
num_of_servers=int(sys.argv[2])
index= int(sys.argv[3])

packetsize=1500
datasize=1400

def server_sends_file(sock, filename, client_addr, port_num, num_of_servers,index):
	#print "Client connected:", client_addr
	file = open(filename, "rb")
	statinfo = os.stat(filename) #get information
	fsize = statinfo.st_size #get size of file
	num_of_packets = (fsize/datasize) + 1	
	num_of_packets_round = int(math.ceil(num_of_packets))

	for i in range(0,num_of_packets_round):
		data = str(file.read(datasize))#read file at beginning
		packet= str(i) + "|" + str(num_of_packets) + "|" + str(len(data)) + "|" + str(fsize) + "888888" + data 
		sleep(.02)
		sock.sendto(packet, client_addr)
	
	#localtime = time.asctime( time.localtime(time.time()) )
	#print "Finished Sending File--->time  ", localtime
	print "Number of packets sent--->  ", num_of_packets_round

def fork_server(port_num,index):
		print "Server waiting for connection on port ",port_num
		message, client_addr = sock.recvfrom(256)
        	print "recieved message"
		server_sends_file (sock, message, client_addr,port_num,num_of_servers,index)
fork_server(port_num, index)

