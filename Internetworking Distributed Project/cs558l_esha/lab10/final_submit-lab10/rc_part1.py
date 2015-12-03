#!/usr/bin/python
from socket import *
import sys, time, string, struct
import os
from sys import argv
from time import sleep
import time, thread

segments_to_expect = 0
recv_dict = {}
proto = 5000
dstAddr = "\xff\xff\xff\xff\xff\xff"
#dstAddr = "\x00\x04\x23\xae\xcc\x12"
srcAddr = ""

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

def run_router():
    global segments_to_expect
    global proto
    prev_seq_no = 0
    something_lost = 0
    loss_count = 0
    old_loss_count = 0
    
    s = socket(AF_PACKET, SOCK_RAW, proto)
    s.bind(("eth2",proto))
    ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
    packets_recieved=0
    outfile = open("out.txt", "w+")
    
    # Receive 'number of segments to expect'
    rxFrame = s.recv(2048)
    rx_curr = extract_frame(rxFrame)
    my_int_seq_no = rx_curr['seq']
    till_here = 16 + rx_curr['len']
    dummy, payload = rxFrame[:16],rxFrame[16:till_here]    
    if (my_int_seq_no == 0):  # Sender telling me how many segments to expect
            segments_to_expect = payload
            segments_to_expect = long(payload)
            print "I am expecting %s segments" % segments_to_expect

    for i in range(1,segments_to_expect+1):     # Init dict
        recv_dict[i] = 0
    
    start = time.time()


    # Start receiving other data
    while(packets_recieved < segments_to_expect):
        rxFrame = s.recv(2048)
        rx_curr = extract_frame(rxFrame)
        my_payload_seq_no = rx_curr['seq']        
        if (my_payload_seq_no == 8999999): # DONE segments_to_expect
            print "Got a 8999999 Sender has finished sending. I received ",packets_recieved
            break

        recv_dict[long(my_payload_seq_no)] = 1 # Mark current segment as received
        
        # Analysis        
        gap = long(my_payload_seq_no) - prev_seq_no
        if(gap > 1): # Something was lost
            something_lost = 1
            for k in range(prev_seq_no+1,prev_seq_no+gap):
                recv_dict[k] = -1       # Set those as unreceived
            
        packets_recieved+=1
        prev_seq_no = long(my_payload_seq_no)
        
        #node_to_send = content_based_route(payload)
        outfile.write(rx_curr['payload'] + '\n')

    print "[Phase I] packets received:", packets_recieved
    
    if(something_lost == 1):
        lost_string = ""
        print "Analysing lost segments"
        f_resend = open('/users/sc558ag/resend.log','w')
        for k in range(1,segments_to_expect+1):
            if(recv_dict[k] == -1): 
                loss_count = loss_count+1
                f_resend.write(str(k)+",")   
        # Send a 0000000 RESEND to sender. Sender will read list from /users/sc558ag/
        f_resend.close()
        txFrame = make_frame(0, len('foo'), proto, 'foo')
        s.send(txFrame)
        
        old_loss_count = loss_count
        print "[Phase I] packets lost:" , loss_count

        ################ PHASE II ########################################
        retry_recv_count = 0 # number of segments received in phase II
        # Start receiving retransmits
        while(retry_recv_count < loss_count):
            rxFrame = s.recv(2048)            # Receive that phaseII segment
            rx_curr = extract_frame(rxFrame)
            my_payload_seq_no = rx_curr['seq']
            if(my_payload_seq_no == 8999999): continue
            outfile.write(rx_curr['payload'] + '\n')
            retry_recv_count = retry_recv_count + 1
            packets_recieved = packets_recieved + 1
            old_loss_count = old_loss_count - 1
    else:
        print "Sender has finished sending. I received ",packets_recieved    
        # Send a 9999999 OK to sender
        txFrame = make_frame(8999999, len('foo'), proto, 'foo')
        s.send(txFrame)
        
    s.close()
    print "[Phase II] packets received:", packets_recieved
    print "[Phase II] packets lost:" , old_loss_count

    end = time.time()
    total_time = end - start
    print "Total time: ", total_time

if __name__=="__main__":
   run_router()
