#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<sstream>
#include <stdlib.h>
using namespace std;

#define PACKETSIZE 1
#define DATASIZE 1450
#define MAXDATALEN 1500

struct message {
   
    int data_type;// 1=CONNECT,2=data
    char data[MAXDATALEN];
    int fileSize;//in data[2]
    int packetNo;//in data[3]
    int bytes_in_msg;//in data[4]
   
};


void packetize(char* filename, vector<message*>& hold_message){
	
 
 	fstream bigFile;
 	ifstream is;
 	int packets, filesize;
 	
 	is.open (filename, ios::binary );
 	
 	// get length of file
  	is.seekg(0, ios::end);
	filesize = is.tellg();
  	is.seekg(0, ios::beg);
  	
  	
  	while(is){
 	  
 	  struct message *packet= new message();
 	  //message->data[0]= 0x01;
 	  
 	   	   	  
 	  is.read(packet->data,PACKETSIZE); //read data from 
 	  hold_message.push_back(packet);
 	
 	}
}



int main(int argc, char* argv[]){

	vector<message*> msg_queue;
	
	char filename[20]="input.in";
	//memset(filename, 0, sizeof filename);
	
	packetize(filename,msg_queue);


	for(int i = msg_queue.size()-1; i>=0; i--){
	
	  cout<<msg_queue.at(i)->data;
	  delete msg_queue.at(i);
	
	}

	cout<<endl;

	return 0;

}
