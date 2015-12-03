#!/usr/bin/python
from socket import *
from sys import argv
import os
import sys, time, string, struct
from time import sleep
import time
import math

my_line_array = []
proto = 5000
s = socket(AF_PACKET,SOCK_RAW,proto)
s.bind(("eth0",proto))
ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
srcAddr = hwAddr                      # My mac addr
dstAddr = "\xff\xff\xff\xff\xff\xff"  # @todo put unicast address
#dstAddr = "\x00\x04\x23\xae\xcf\xf7"

def make_frame(packet_number, payload_size, proto, payload):
    global srcAddr, dstAddr
    srcAddr = "%06x" % packet_number  #########
    payload_size_str = "%02d" % payload_size
    packet =  payload_size_str + payload
    txFrame = struct.pack("!6s6sh",dstAddr,srcAddr,proto) + packet
    #print txFrame[6:]
    return txFrame

def extract_frame(rxFrame):
    dict_to_return = {}
    #print "[raw] esha_seq_no: ",rxFrame[6:12]
    dict_to_return['seq'] = int(rxFrame[6:12],16)#Convert hex to int
    dict_to_return['len'] = long(rxFrame[14:16])#Length of payload
    dict_to_return['payload'] = rxFrame[16:]
    return dict_to_return

def resend_list(s, list_of_lost):
    global my_line_array
    for lost in list_of_lost:
        #Now resending 'lost'
        if lost:
            line = my_line_array[long(lost)-1]
            line = line.rstrip()
            #print "Resending %d ::  %s" % (long(lost),line)
            txFrame = make_frame(long(lost), len(line), proto,line)
            s.send(txFrame)
            time.sleep(1/1000000.0) #(*10^-7) best 1-7

def run_server():
    global my_line_array
    file = open("file_50m.txt", "rb")
    filename="file_50m.txt"
    statinfo = os.stat(filename) #get information
    fsize = statinfo.st_size #get size of file
    my_line_array = file.readlines()
    total_lines_in_file = len(my_line_array)
    print "Total lines in file: %d" % total_lines_in_file
    
    total_pckts=0
    i=0
    # Tell receiver how many segments to expect   
    txFrame = make_frame(0, len(str(total_lines_in_file)), proto, str(total_lines_in_file))  
    s.send(txFrame)
  
    # Actual file sending loop
    for line in my_line_array:
        line=line.rstrip()
        payload_size = len(line)
        i=i+1
        #print "Sending %d" % i
        #if i == 1: continue
        #if i == 2: continue
        txFrame = make_frame(i, payload_size, proto, line)
        s.send(txFrame)     # Send it              
        total_pckts=total_pckts + 1	
        time.sleep(1/10000000.0) #(*10^-7) best 1-6
        

    # File ends, send finish segment 5o times
    for k in range(1,50):
        txFrame = make_frame(8999999, len('foo'), proto, 'foo')
        #print "Sending DONE segment"
        s.send(txFrame)
    
    print "Number of packets sent--->  ", total_pckts   
    ####################### PHASE II #####################################
    # Now see how it went
    rxFrame = s.recv(2048)
    print "Received some news from original receiver"
    rx_curr = extract_frame(rxFrame)
    my_payload_seq_no = rx_curr['seq']
    if(my_payload_seq_no == 0):
        # Loss happened. Resend part
        f_resend = open('/users/sc558ag/resend.log','rb')
        #new_payload = f_resend.read()        
        list_of_lost =  f_resend.read().split(',')
        f_resend.close()
        till_here = len(list_of_lost) - 1
        list_of_lost = list_of_lost[:till_here]
        resend_list(s, list_of_lost)    # Resend these segments
    elif (my_payload_seq_no == 8999999):
        # Success
        print "All segments sent successfully"
    s.close()

if __name__=="__main__":
        run_server()
