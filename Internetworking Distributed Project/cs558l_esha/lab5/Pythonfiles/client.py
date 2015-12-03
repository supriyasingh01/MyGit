#!/usr/bin/python

from socket import *
import sys, time
from thread import start_new_thread, get_ident
import os
import stat
start = time.time()
threads = {}
MAXDATALEN=3
L=[]

fin=open(sys.argv[2],'r')
#fout=open('outputfile.out','a')
statinfo=os.stat(sys.argv[2])
finsize=statinfo.st_size
print finsize

for i in range(0,finsize/MAXDATALEN):
		strng=fin.read(MAXDATALEN)
		fin.seek(0,1)
		#print strng		
		L.append(strng)
		
print L

									#code for sending metadata via TCP
									
									
sock = socket(AF_INET, SOCK_DGRAM)
def request(n):
		sock.sendto("%s" % (L.pop(0)),(sys.argv[1], int(sys.argv[3]))) #sending data via UDP
		messin, server = sock.recvfrom(255)
		print "Received:", messin
		#print "from:",server
		#fout.write(messin)
		del threads[get_ident()]

for n in range(finsize/MAXDATALEN):
		id = start_new_thread(request, (n,))
		threads[id] = None
		#print id,
while threads: time.sleep(.1)
sock.close()
print "%.2f seconds" % (time.time()-start)