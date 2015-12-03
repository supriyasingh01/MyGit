#!/usr/bin/python
from socket import *
import sys, time
from sys import argv
import os
import stat
import hashlib


def packets(filename, buf_size, data_size):
	file = open(filename, "rb") #open the file
	statinfo = os.stat(filename) #get information
	fsize = statinfo.st_size #get size of file
	print "fsize is:",fsize
	
	#keep track of total number of packets, checksum, sequence number
        #total file size
	
	packet_List1 = list()#create list for all packets
	packet_List2=list()
	packet_List3=list()
	packet_List4=list()
	packet_List=list()
	sequence_number = 0#keep track of packet sequence
	num_of_packets = round(fsize/data_size)	
	print "no. of packets is:",num_of_packets
	data = file.read(data_size)#read file at beginning
	
	while(len(data) >= data_size):#create each packet along with relevant information
		packet=[]
		sequence_number+=1
		#print sequence_number
		packet.append(sequence_number) #inserts sequence number
		packet.append(num_of_packets)  #inserts total number of packets
		#packet.append(get_checksum(data)) #inserts checksum
		packet.append(data) #inserts data		

		packet_List.append(packet)#packetizes file, stores in list
		data = file.read(data_size)
		file.seek(0,1)

	return packet_List

def get_checksum(checksum_data):

	md5.new(checksum_data).digest()
	return  md5
