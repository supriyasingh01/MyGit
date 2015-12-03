#!/usr/bin/python

import sys
import string
import struct
from socket import *

proto = 0x55aa

s = socket(AF_PACKET, SOCK_RAW, proto)
s.bind(("eth0",proto))

ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()

#srcAddr = hwAddr
#dstAddr = "\x00\x04\x23\xc7\xa8\x0a"
#ethData = "here is some data for an ethernet packet"


rxFrame = s.recv(2048)

dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])
ethData = rxFrame[14:]

print "Rx[%d]: "%len(ethData) + string.join(["%02x"%ord(b) for b in ethData]," ")



s.close()
