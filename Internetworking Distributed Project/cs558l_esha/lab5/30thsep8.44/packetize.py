#!/usr/bin/python
from socket import *
import sys, time
from sys import argv
import os
import stat
import hashlib
import math

def get_checksum(checksum_data):

	md5= hashlib.md5()
	md5.update(checksum_data)

        return md5.digest()

def packets(filename, buf_size, data_size,num_of_servers, index):

	
	file = open(filename, "rb") #open the file
	statinfo = os.stat(filename) #get information
	fsize = statinfo.st_size #get size of file
	
	#stepsize = int((fsize / num_of_servers)) 	
	#forward = stepsize * index
	#print "Numof servers: ", num_of_servers, " Fsize: ",fsize," Stepsize: ", stepsize, " index: ",index
	
	#keep track of total number of packets, checksum, sequence number
        #total file size
	packet_queue = list()#create list for all packets
	sequence_number = 0#keep track of packet sequence
	num_of_packets = (fsize/data_size) + 1	

	#file.seek(forward)
	

	#while(len(data) >= data_size):#create each packet along with relevant information
	for i in range(0, int(math.ceil(num_of_packets))):

		packet=[]
		data = file.read(data_size)#read file at beginning	
		sequence_number+=1
		
		packet.append(sequence_number) # 0 inserts sequence number
		packet.append(num_of_packets)  # 1 inserts total number of packets
		#packet.append(get_checksum(data)) # 2 inserts checksum
		packet.append(len(data)) # 3 insert data length
		packet.append(fsize) # 4 inserts file size
		packet.append(data) # 5 inserts data		

		#print index,"Values of what Im sending", data

		packet_queue.append(packet)#packetizes file, stores in list
		#file.seek(0, 1)		


	return packet_queue
