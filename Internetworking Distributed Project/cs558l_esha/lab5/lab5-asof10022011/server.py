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
index= int(sys.argv[3])
temp = open('tip%d'%index,'w')
temp.write("_")


def fork_server(port_num,index):
	print "Server waiting for connection on port ",port_num
	message, client_addr = sock.recvfrom(256)
       	print "recieved message"
	server_sends_file (sock, message, client_addr,port_num,num_of_servers,index)

fork_server(port_num, index)
