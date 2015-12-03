#!/usr/bin/python
import os
import stat
MAXDATALEN=3
L=[]

f=open('file','r')
statinfo=os.stat('file')
fsize=statinfo.st_size
print fsize

for i in range(0,fsize/MAXDATALEN):
		strng=f.read(MAXDATALEN)
		f.seek(0,1)
		#print strng
		L.append(strng)

print L		
print "Packet is:", L.pop(3);



