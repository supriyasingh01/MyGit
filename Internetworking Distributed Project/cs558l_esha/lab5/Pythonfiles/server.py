#!/usr/bin/python
from socket import *
from sys import argv
from thread import start_new_thread
fout=open('outputfile.out','a')

def lengthy_action(sock, message, client_addr):
		from time import sleep
		print "Client connected:", client_addr
		print message
		fout.write(message)
		#sleep(5)
		sock.sendto(message.upper(), client_addr)

sock = socket(AF_INET, SOCK_DGRAM)
sock.bind(('',int(argv[1])))
print "server waiting for connections"
while 1: # Run until cancelled
		message, client_addr = sock.recvfrom(256)
		start_new_thread(lengthy_action, (sock, message, client_addr))