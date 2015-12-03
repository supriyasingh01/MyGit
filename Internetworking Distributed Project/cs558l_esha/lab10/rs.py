#!/usr/bin/python
from socket import *
from sys import argv
import os
import sys, time, string, struct
from time import sleep
import time
import math

proto = 0x55aa
s = socket(AF_PACKET,SOCK_RAW,proto)
s.bind(("eth3",proto))  
ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
srcAddr = hwAddr
dstAddr = "\x00\x04\x23\xa6\x57\x99"
datasize=50
#total_pckts=0

def run_server():
        file = open("file2.txt", "rb")
        filename="file2.txt"
        statinfo = os.stat(filename) #get information
        fsize = statinfo.st_size #get size of file
        num_of_packets = int(math.ceil(fsize/datasize))
        total_pckts=0
        for i in range(0,num_of_packets):
               	ethData = str(file.read(datasize))#read file at beginning
               	packet_number = "%03d" % i  # packet number 3 bytes 
         	packet=  str(packet_number) + ethData
		total_pckts=total_pckts+1	
                txFrame = struct.pack("!6s6sh",dstAddr,srcAddr,proto) + packet	
                print "Tx[%d]: "%len(packet) + string.join(["%02x"%ord(b) for b in packet]    ," ")
                s.send(txFrame)
        print "Number of packets sent--->  ", total_pckts
        s.close()

if __name__=="__main__":
        run_server()
