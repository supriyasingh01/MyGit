#!/usr/bin/python
from socket import *
from sys import argv
from thread import start_new_thread
from threads import *
import os

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(('',int(argv[1])))


port_num= int(sys.argv[1])
num_of_servers=int(sys.argv[2])
index=0

def fork_server(port_num,index):
	child_pid = os.fork()
    	if child_pid==0:
		print "Server waiting for connection on port ",port_num
		message, client_addr = sock.recvfrom(256)
        	print "recieved message"
		start_new_thread(server_sends_file, (sock, message, client_addr,port_num,index))
	else:
		print "This is a parent process"

for x in range(0, num_of_servers):
	fork_server(port_num+x,x)


while(1):
	1
