#!/usr/bin/python
#!/usr/bin/python
from socket import *
import sys, time, string, struct
import os
from sys import argv
from time import sleep
import time, thread
start = time.time()

#print "Rx[%d]: "%len(ethData) + string.join(["%02x"%ord(b) for b in ethData]," ")

#Globals here
mac_dict = {}

def init():
    global mac_dict 
    proto = 5001
    fcfg = open('config_file.txt','rb')
    for line in fcfg:
        # Create  raw socket for this destination
        line = line.rstrip()
        if len(line) == 0: break
        print "Processing " + line
        node_pattern,node_info = line.split(':')
        mac_dict[node_pattern] = {}
        if node_pattern == 'rtr':
            pass
        else:
            node_mac, node_if, listen_if = node_info.split(',')
            #print "[Init] Pattern: " + node_pattern + " I/f: " + node_if + " Mac: " + node_mac      
            print "Binding to interface: " + listen_if
            mac_dict[node_pattern]['listen_if'] = listen_if
        proto = proto + 1
    fcfg.close()

def run_router():
   init()
   global mac_dict
   proto = 5000
   s = socket(AF_PACKET, SOCK_RAW, proto)
   hostname = gethostname().split('.')[0]
   hostname = hostname[1:]
   print "I am " + hostname
   s.bind((mac_dict[hostname]['listen_if'], proto))
   ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
   packets_recieved=0
   out_file_name = hostname + '.out'
   outfile = open(out_file_name, "w+")

   while(1):
       rxFrame = s.recv(2048)
       # First 14 bytes
       dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])
       #my_ether_hdr=dstAddr+srcAddr+proto,my_payload_hdr=packet_number+payload_size
       my_ether_hdr,my_payload_hdr,payload = rxFrame[:14], rxFrame[14:22],rxFrame[22:]
       print "Rx: " + my_payload_hdr + payload
       packets_recieved+=1
       print "packets received:", packets_recieved
       outfile.write(payload + '\n')
   s.close()
   
if __name__=="__main__":
   run_router()
