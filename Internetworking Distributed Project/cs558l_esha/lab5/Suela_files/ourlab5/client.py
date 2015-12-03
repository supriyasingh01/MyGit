#!/usr/bin/python
from socket import *
import sys, time
from thread import start_new_thread, get_ident
from threads import *

start = time.time()
threads = {}
sock = socket(AF_INET, SOCK_DGRAM)

#for n in range(3):
id = start_new_thread(request, (sock, threads))
threads[id] = None
#print id,
while threads: time.sleep(.0)
sock.close()
print "%.2f seconds" % (time.time()-start)
