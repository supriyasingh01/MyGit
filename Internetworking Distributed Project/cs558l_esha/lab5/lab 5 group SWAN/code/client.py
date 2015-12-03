#!/usr/bin/python
from socket import *
import sys, time, string
import os
from sys import argv
from time import sleep
import time

start = time.time()
threads = {}

port_num= int(sys.argv[3])
num_of_clients=int(sys.argv[4])
index=int(sys.argv[5])


recsize=1500

def client_requests_file(n, sock,threads,index,num_of_servers):

	recsize =1500
	messin, server = sock.recvfrom(recsize)
	
	packets_recieved = 1
	p=string.split(messin, "888888")
	header, payload = p[0], p[1]

	s=string.split(header, "|")
	
	print "HEADER", header, "CLIENT:", index
	#print "LOAD", payload,  "sequence number:", s[0], "number of packets:", s[1], "datalength:", s[2], "filesize:", s[3] 
	#print "sequence number", string.lstrip(p[0])
	
	sequence_number = s[0]
	num_of_packets = s[1]
	datalength = s[2]
	filesize = s[3]
	
	while(packets_recieved < int(s[1])):
		sleep(.001)
		messin, server = sock.recvfrom(recsize)
		packets_recieved+=1
	print "Total Number of packets is ", packets_recieved

	localtime = time.asctime( time.localtime(time.time()) )
	#print "Finished Recieving time :", localtime
	#outfilename = 'outfile.out-%d'%index
	#outfile = open(outfilename, "w+")
	#outfile.write(file)


def fork_client( n, port_num):
	sock = socket(AF_INET, SOCK_DGRAM)
	#print "Client talking to ", port_num," "
	sock.sendto("%s" % (sys.argv[2]),(sys.argv[1], port_num))
	client_requests_file (n,sock,threads,index, num_of_clients)
	threads[id] = None
	print "%.2f seconds" % (time.time()-start)
	sock.close()

fork_client(0,port_num+0)
#command= './join.py -i %s -j ' % 'outfile.out'

#if index ==  num_of_clients -1:
	#sleep(1)
	#os.system(command)

