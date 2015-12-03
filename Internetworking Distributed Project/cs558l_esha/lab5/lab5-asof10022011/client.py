#!/usr/bin/python
from socket import *
import sys, time
#from thread import start_new_thread, get_ident
from threads import *

start = time.time()
threads = {}

port_num= int(sys.argv[3])
num_of_clients=int(sys.argv[4])
index=int(sys.argv[5])




def fork_client( n, port_num):
	sock = socket(AF_INET, SOCK_DGRAM)
	print "Client talking to ", port_num," "
	sock.sendto("%s" % (sys.argv[2]),(sys.argv[1], port_num))

	client_requests_file (n,sock,index,num_of_clients)
	sock.close()
	print "%.2f seconds" % (time.time()-start)

fork_client(0,port_num+0)
sys.exit(0)  
