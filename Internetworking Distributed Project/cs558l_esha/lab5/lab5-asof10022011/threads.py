#!usr/bin/python
from socket import *
import sys, time
from sys import argv
from thread import start_new_thread, get_ident
from packetize import *
from time import sleep
import time
import pickle

packetsize=1500
datasize=1400
recsize=packetsize+datasize

def server_sends_file(sock, filename, client_addr, port_num, num_of_servers,index):
	print "Client connected:", client_addr
	file = open(filename, "rb")
	print "Name of File is client is requesting: ",filename
	packet_List  = packets(filename,packetsize, datasize, num_of_servers,index)#sends to function to break up and return list of packets
	packet_List.reverse()
	#print "Size of packet list", len(packet_List)
	
	print "Starting to send all the packets "


	#print time.localtime()
	while(len(packet_List)!= 0):		
		packet =str(packet_List.pop()) 
		#print "Packet Im sending ",packet[0] 
		#sleep(0.3)
		#packet_data = pickle.dumps(packet)
		sock.sendto(packet, client_addr)
	
	#print time.localtime()
	print "Finished Sending File  "

	temp = open('tip%d'%index,'w')
	temp.write(str(index))
	#print "printing after server writes", temp

def client_requests_file(n,sock,index,num_of_servers):

	recsize =1500
	#print "Now in client talking to ", 
	messin, server = sock.recvfrom(recsize)
	#packet_data = pickle.loads(messin)
	print "Received messin:", messin
	#num_of_packets = packet_data[1]
	#message = messin[1:-1].split(',')#parse the string into array		
	
	#num_of_packets = (int)(float(message[1])) #set num of packets to recieve
	packets_recieved = 1
	file= ""
	#fdata = packet_data[4]
	#file= fdata
	#file += fdata[2:(len(fdata)-1)]

	#print "Total Number of packets is ", messin[1]
	temp = open('tip%d'%index,'r')
	#print tip
	go="-"
	#while(packets_recieved <= num_of_packets-1):
	while(packets_recieved<1498):
	#while(0):
		#print "Received:", messin[4],",",packets_recieved,", ",num_of_packets
		#print num_of_packets/num_of_servers
		messin, server = sock.recvfrom(recsize)
		#print "messin is:",messin
		#packet_data = pickle.loads(messin)
		#message = messin[1:-1].split(',')
		file += messin

		packets_recieved+=1
	
		#file += fdata[2:(len(fdata)-1)]	
		#print "Data here is ", packet_data[4]       
		go=temp.read(1)
		print time.localtime()
		print "++++++++++++++++++++++++++++"
		print "value of looop is", go

	#print time.localtime()	
	#print "Complete file is", file
	outfilename = 'files/outfile%d.out'%index
	outfile = open(outfilename, "w+")
	outfile.write(file)

	#del threads[get_ident()]
