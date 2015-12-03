#!/usr/bin/python
from socket import *
import sys, time, string, struct
import os
from sys import argv
from time import sleep
import time, thread

start = time.time()
recsize=100
packets_recieved=0

def run_client():
        proto = 0x55aa
        s = socket(AF_PACKET, SOCK_RAW,proto)
        s.bind(("eth4",proto))
        ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
#        rxFrame = s.recv(2048)
#        dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])
        packets_recieved=0
        outfile = open("out.txt", "w+")

        while(packets_recieved <5):
               	rxFrame = s.recv(2048)
                dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])
                header, payload = rxFrame[14:17], rxFrame[17:66]
                print "Rx[%d]: "%len(payload) + string.join(["%02x"%ord(b) for b in payload]    ," ")

                print "sequence_number", header
                packets_recieved+=1

                outfile.write(payload)
        s.close()
        print "packet received:", packets_recieved
if __name__=="__main__":
        run_client()
