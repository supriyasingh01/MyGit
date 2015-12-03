#!/usr/bin/python
from socket import *
from sys import argv
import os
import sys, time, string, struct
from time import sleep
import time
import math

proto = 5000
s = socket(AF_PACKET,SOCK_RAW,proto)
s.bind(("eth2",proto))
#s.bind(("eth5",proto))
ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
srcAddr = hwAddr                      # My mac addr
dstAddr = "\x00\x15\x17\x1c\xd8\x66"
#dstAddr = "\x00\x15\x17\x1e\x03\x02"

def run_server():
    file = open("file1.txt", "rb")
    filename="file1.txt"
    statinfo = os.stat(filename) #get information
    fsize = statinfo.st_size #get size of file
    total_pckts=0
    i=0
    for line in file:
        i=i+1
        line=line.rstrip()
        payload_size = len(line)
        if payload_size == 0:
            break
        packet_number = "%05d" % i  # packet number 3 bytes 
        payload_size_str = "%03d" % payload_size
        proto_str = "%05d" % proto
        packet = str(packet_number) + str(payload_size_str) + line
        total_pckts=total_pckts+1	
        #   make raw packet with router as dest mac
        txFrame = struct.pack("!6s6sh",dstAddr,srcAddr,proto) + packet	
        print "Tx: " + packet
        s.send(txFrame)#   send it        
    print "Number of packets sent--->  ", total_pckts
    s.close()

if __name__=="__main__":
        run_server()
