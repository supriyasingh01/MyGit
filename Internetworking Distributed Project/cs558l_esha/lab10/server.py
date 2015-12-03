#!/usr/bin/python

import sys
import string
import struct
import csv
from socket import *

proto = 0x55aa

s = socket(AF_PACKET, SOCK_RAW, proto)
s.bind(("eth0",proto))

ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()

srcAddr = hwAddr
dstAddr = "\x00\x04\x23\xc7\xa6\x34"
f1=csv.reader(open('file1.txt','r'),delimiter='\t')
for row in f1:
    packet=row
    ethData=str(packet)
    txFrame = struct.pack("!6s6sh",dstAddr,srcAddr,proto) + ethData

print "Tx[%d]: "%len(ethData) + string.join(["%02x"%ord(b) for b in ethData]," ")
                      
s.send(txFrame)

s.close()
