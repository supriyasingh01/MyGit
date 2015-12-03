#!/usr/bin/python
from socket import *
import sys, time
from sys import argv
import os
import stat
import hashlib
import math


index=int(sys.argv[2])


outfile = open("testfiles/esha.out", "w+")


file = open(sys.argv[1], "rb") #open the file
statinfo = os.stat("input.in") #get information
fsize = statinfo.st_size #get size of file

stepsize = math.ceil(fsize / 5)


ans = int(stepsize)*index
file.seek(ans)

buffer = file.read(int(stepsize))
outfile.write(buffer)
