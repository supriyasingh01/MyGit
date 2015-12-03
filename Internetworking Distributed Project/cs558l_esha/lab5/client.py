#!/usr/bin/python
from socket import *
import sys, time
from thread import start_new_thread, get_ident
from threads import *

start = time.time()
threads = {}
sock = socket(AF_INET, SOCK_DGRAM)

for n in range(1):

	id = start_new_thread(client_requests_file, (n,sock, threads))
	threads[id] = None
	#print id,


while threads: time.sleep(.1)
sock.close()
print "%.2f seconds" % (time.time()-start)
