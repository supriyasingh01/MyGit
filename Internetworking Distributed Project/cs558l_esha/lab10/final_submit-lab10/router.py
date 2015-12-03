#!/usr/bin/python
from socket import *
import sys, time, string, struct
import os
from sys import argv
from time import sleep
import time, thread
import re

#Globals here
mac_dict = {}

start = time.time()

def ByteToHex( byteStr ):
    """
    Convert a byte string to it's hex string representation e.g. for output.
    """
    
    # Uses list comprehension which is a fractionally faster implementation than
    # the alternative, more readable, implementation below
    #   
    #    hex = []
    #    for aChar in byteStr:
    #        hex.append( "%02X " % ord( aChar ) )
    #
    #    return ''.join( hex ).strip()        

    return ''.join( [ "%02X " % ord( x ) for x in byteStr ] ).strip()

def HexToByte( hexStr ):
    """
    Convert a string hex byte values into a byte string. The Hex Byte values may
    or may not be space separated.
    """
    # The list comprehension implementation is fractionally slower in this case    
    #
    #    hexStr = ''.join( hexStr.split(" ") )
    #    return ''.join( ["%c" % chr( int ( hexStr[i:i+2],16 ) ) \
    #                                   for i in range(0, len( hexStr ), 2) ] )
 
    bytes = []

    hexStr = ''.join( hexStr.split(" ") )

    for i in range(0, len(hexStr), 2):
        bytes.append( chr( int (hexStr[i:i+2], 16 ) ) )

    return ''.join( bytes )

# Find out whom to send based on content of string
# Returns a tuple (interface,mac_address) to which we should send the packet
def content_based_route(my_payload_hdr, payload): 
    global mac_dict
    destNode = ""
    if (re.search('red',payload,re.IGNORECASE)):
        destNode = 'red'
    elif (re.search('blue',payload,re.IGNORECASE)):
        destNode = 'blue'
    elif (re.search('green',payload,re.IGNORECASE)):
        destNode = 'green'
    elif (re.search('yellow',payload,re.IGNORECASE)):
        destNode = 'yellow'

    print "Found :" + destNode
    if(destNode):
        # Send to this destination node
        print "Sending to " + destNode + " on " + mac_dict[destNode]['local_if'] + " mac: " +mac_dict[destNode]['dest_mac']
        my_socket = mac_dict[destNode]['socket']
        ifName,ifProto,pktType,hwType,hwAddr = my_socket.getsockname()
        srcAddr = hwAddr
        txFrame = struct.pack("!6s6sh",HexToByte(mac_dict[destNode]['dest_mac']),srcAddr,5000) + my_payload_hdr + payload
        #txFrame = struct.pack("!6s6sh",'\x00\x15\x17\x1e\x03\x02',srcAddr,5000) + payload
        my_socket.send(txFrame)
    return destNode


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
        mac_dict[node_pattern]['socket'] = socket(AF_PACKET, SOCK_RAW, proto)   #Create
        if node_pattern == 'rtr':
            node_mac, node_if, init_src = node_info.split(',') 
            #print "[Init] Pattern: " + node_pattern + " I/f: " + node_if + " Mac: " + node_mac + " Init I/f: " + init_src
            mac_dict[node_pattern]['listen_if'] = init_src
        else:
            node_mac, node_if, listen_if = node_info.split(',')
            #print "[Init] Pattern: " + node_pattern + " I/f: " + node_if + " Mac: " + node_mac      
            print "Binding to interface: " + node_if
            mac_dict[node_pattern]['socket'].bind((node_if, proto))                   # Bind
        mac_dict[node_pattern]['dest_mac'] = node_mac
        mac_dict[node_pattern]['local_if'] = node_if
        proto = proto + 1
    fcfg.close()
    
def run_router():   
    global mac_dict
    proto = 5000
    init()      # Read info about other nodes
    #s = socket(AF_PACKET, SOCK_RAW, proto)
    s = mac_dict['rtr']['socket']
    s.bind((mac_dict['rtr']['listen_if'],5000))
    #ifName,ifProto,pktType,hwType,hwAddr = mac_dict['rtr']['socket'].getsockname()
    ifName,ifProto,pktType,hwType,hwAddr = s.getsockname()
    packets_recieved=0
    while(1):
        #rxFrame =  mac_dict['rtr']['socket'].recv(2048)
        rxFrame =  s.recv(2048)
        dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])       # First 14 bytes
        #my_ether_hdr=dstAddr+srcAddr+proto,my_payload_hdr=packet_number+payload_size
        my_ether_hdr,my_payload_hdr,payload = rxFrame[:14], rxFrame[14:22],rxFrame[22:]
        print "Rx: " + my_payload_hdr + payload
        packets_recieved+=1
        print "packets received:", packets_recieved
        node_to_send = content_based_route(my_payload_hdr, payload)
    s.close()
    print "packets received:", packets_recieved
    
if __name__=="__main__":
   run_router()
