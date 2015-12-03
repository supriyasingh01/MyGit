#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <map>
#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4955"    // the port users will be connecting to
//using namespace std;

#define TRAINING_FLOWS 6000
#define ATTACK_TYPES 7
#define MAX_FLOWS 1000				//max flows in the flow table


/* 
*****ATTENTION*****
To add a new attack type to the classifier -
1. Increment ATTACK_TYPES by 1
2. Add attack name at the end of "attacks" array
3. Update attack codes in comments below
*/


/*
ATTACK CODES
in struct flowStats
0 - normal / no attack
1 - tcp syn (tcp)
2 - smurf (icmp)
3 - fraggle (udp)
*/

typedef struct
{
	//char protocol[10];
	int protocol;
	int packets;
	int bytes;
	int duration;
	int srcPort;
	int dstPort;
	char srcIP[17];
	char dstIP[17];
	int attack;				
	int training;
}flowStats;

typedef struct
{
	long double apfMean;
	long double abfMean;
	long double adfMean;
	long double apfVar;
	long double abfVar;
	long double adfVar;
	long double udp;
	long double icmp;
	long double tcp;
	long double other; //any other protocol than tcp,udp,icmp
	int numPairs;
	long double smallPairClass;		//small num pairs
	long double mediumPairClass;		//medium num pairs
	long double largePairClass;		//large num pairs
	long double singleGrowth;		//growth of single pairs
	long double smallSingleClass;		//small num pairs
	long double largeSingleClass;		//large num pairs	
	long double portGrowth;
	long double smallPortClass;		//small port growth
	long double largePortClass;		//large port growth
}classifier;


const char *attacks[ATTACK_TYPES] = {"NORMAL","SYN FLOOD","SMURF","FRAGGLE","TCP SYN-FIN", "UDP FLOOD", "ICMP FLOOD"};

flowStats flows[TRAINING_FLOWS];
flowStats rcv[TRAINING_FLOWS];
classifier clas[ATTACK_TYPES];
int num_flows;
int rec_pairs;
double median_b, median_d, median_p;
int num_tflows;			//number of flow entries in the flow table received
double num_attacks[ATTACK_TYPES];
double attack_duration[ATTACK_TYPES];
double rec_duration;
long double prob_attack[ATTACK_TYPES];		//probability of the attack type
double num_normal;
struct timeval start,end;
double t1,t2,learningTime;
int lastAttack;
int lastAttackCounter;
int numOffendingFlows;
struct sockaddr_un local, remote;
int s,s2,s3, len;
unsigned int len2;

//
int sockfd;
struct addrinfo hints, *servinfo, *p;
int rv;
int numbytes;
//

void removeFlow(int protocol,int srcPort,int dstPort,char srcIP[17],char dstIP[17]);

//for growth of ports calculation
std::map<int,int> ports;	
	
void validate(char line[1024])
{
	char * token;
	int num=0;
	token = strtok (line,",");
	while (token != NULL)
	{
		if(num==0)	//protocol
		{
			flows[num_flows].protocol=atoi(token);
			//strcpy(flows[num_flows].protocol,token);
		}
		else if(num==1)	//packets
		{
			flows[num_flows].packets=atoi(token);
		}
		else if(num==2)	//bytes
		{
			flows[num_flows].bytes=atoi(token);
			//printf("%s\n",token);
		}
		else if(num==3)	//duration
		{
			flows[num_flows].duration=atoi(token);
			//printf("%d\n",flows[num_flows].duration);
		}
		else if(num==4)	//src port
		{
			flows[num_flows].srcPort=atoi(token);
		}		
		else if(num==5)	//dst port
		{
			flows[num_flows].dstPort=atoi(token);		
		}
		else if(num==6)	//source ip
		{
			strcpy(flows[num_flows].srcIP,token);
		}
		else if(num==7)	//dst ip
		{
			strcpy(flows[num_flows].dstIP,token);
		}
		else	//attack type
		{
			flows[num_flows].attack=atoi(token);
			num_attacks[flows[num_flows].attack]++;
			attack_duration[flows[num_flows].attack]=flows[num_flows].duration+attack_duration[flows[num_flows].attack];
		}
		flows[num_flows].training=1;
		num++;
		token = strtok (NULL, ",");
	}
	//printf("%s,%d,%d,%d,%d,%s,%s,%d,%d\n",flows[num_flows].protocol,flows[num_flows].packets,flows[num_flows].bytes,flows[num_flows].duration,flows[num_flows].dstPort,flows[num_flows].srcIP,flows[num_flows].dstIP,flows[num_flows].attack,flows[num_flows].training);
}

//for calculating the number of pair flows for each attack from the training set
void pairFlowsTraining()
{
	for(int k=0; k<ATTACK_TYPES; k++)
	{
		clas[k].numPairs=0;
	}
	
	for(int k=0; k<ATTACK_TYPES; k++)
	{
		for(int i=0; i<num_flows-1; i++)
		{
			for(int j=i; j<num_flows; j++)
			{
				//if(flows[i].attack==k && strcmp(flows[i].protocol, flows[j].protocol)==0)
				if(flows[i].attack==k && flows[i].protocol==flows[j].protocol)
				{
					if((strcmp(flows[i].srcIP, flows[j].dstIP) == 0) && (strcmp(flows[i].dstIP, flows[j].srcIP) == 0))
					{
						clas[k].numPairs++;
						break;
					}
				}
			}
		}
	}
	for(int k=0; k<ATTACK_TYPES; k++)
	{
		if(clas[k].numPairs/num_attacks[k]>0.6)
		{
			clas[k].smallPairClass=0.00001;
			clas[k].mediumPairClass=0.00001;
			clas[k].largePairClass=1;
			//printf("Pair flows for %d are %d (large)\n",k,clas[k].numPairs);
		}
		else if(clas[k].numPairs/num_attacks[k]>0.4)
		{
			clas[k].smallPairClass=0.00001;
			clas[k].mediumPairClass=1;
			clas[k].largePairClass=0.00001;
			//printf("Pair flows for %d are %d (medium)\n",k,clas[k].numPairs);
		}	
		else
		{
			clas[k].smallPairClass=1;
			clas[k].mediumPairClass=0.00001;
			clas[k].largePairClass=0.00001;
			//printf("Pair flows for %d are %d (small)\n",k,clas[k].numPairs);
		}
		//printf("Pair flows for %d are %d\n",k,clas[k].numPairs);
	}
}

//for calculating the pair flows from the flow table received
int pairFlows()
{
	rec_pairs=0;
	for(int i=0; i<num_tflows-1; i++)
	{
		for(int j=i; j<num_tflows; j++)
		{
			//if(strcmp(rcv[i].protocol, rcv[j].protocol)==0)
			if(rcv[i].protocol==rcv[j].protocol)
			{
				if((strcmp(rcv[i].srcIP, rcv[j].dstIP) == 0) && (strcmp(rcv[i].dstIP, rcv[j].srcIP) == 0))
				{
					rec_pairs++;
					break;
				}
			}
		}
	}	
	//printf("Pairs in the recieved flow are %d\n",rec_pairs);
	if(rec_pairs/num_tflows>0.8)
		return 2;						//large no. of pair flows
	else if(rec_pairs/num_tflows>0.4)
		return 1;						//medium no. of pair flows
	else 
		return 0;						//small no. of pair flows
}

int singleFlows()
{

	int numPair=rec_pairs/2;
	long double singleGrowth=num_tflows-2*numPair;
	singleGrowth=singleGrowth/num_tflows;
	if(singleGrowth>0.6)
	{
		//printf("Single flow growth is %Lf (large)\n",singleGrowth);
		return 1;
	}
	else
	{
		//printf("Single flow growth is %Lf (small)\n",singleGrowth);
		return 0;	
	}		
}


void singleFlowsTraining()
{
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		int numPair=clas[i].numPairs/2;
		clas[i].singleGrowth=num_attacks[i]-2*numPair;
		clas[i].singleGrowth=clas[i].singleGrowth/num_attacks[i];
		if(clas[i].singleGrowth>0.6)			//large port growth
		{
			clas[i].largePortClass=1;
			clas[i].smallPortClass=0.0001;
			//printf("Single flow growth is %Lf (large)\n",clas[i].singleGrowth);
		}
		else
		{
			clas[i].smallPortClass=1;
			clas[i].largePortClass=0.0001;			
			//printf("Single flow growth is %Lf (small)\n",clas[i].singleGrowth);
		}		
	}
}

int portsGrowth()
{
	int num_ports;
	//map<int,int> ports;
	ports.clear();
	for(int j=0; j<num_tflows; j++)
	{
		ports[flows[j].dstPort]=0;	
	}
//	return (int)ports.size();
	
	//long double temp=ports.size()/rec_duration;										//what is the reasonable amt of port growth in 5 sec?
	long double temp=ports.size()/num_tflows;
	if(temp>0.2)																		///MODIFY!!!!
	{	
		//printf("Growth of ports is %Lf (large)\n",temp);
		return 1;
	}
	else
	{
		//printf("Growth of ports is %Lf (small)\n",temp);
		return 0;
	}
}

void portsGrowthTraining()
{
	//map<int,int> ports;
	for(int k=0; k<ATTACK_TYPES; k++)
	{
		ports.clear();
		for(int i=0; i<num_flows; i++)
		{
			if(flows[i].attack==k)
				ports[flows[i].dstPort]=0;	
		}
		clas[k].portGrowth=(double)ports.size();
		//long double temp=clas[k].portGrowth/attack_duration[k];
		long double temp=clas[k].portGrowth/num_attacks[k];
		if(temp>0.02)	
		{
			clas[k].smallPortClass=0.00001;
			clas[k].largePortClass=1;
			//printf("No. of ports: %Lf (large)\n",clas[k].portGrowth);
		}
		else
		{
			clas[k].smallPortClass=1;
			clas[k].largePortClass=0.00001;		
			//printf("No. of ports: %Lf (small)\n",clas[k].portGrowth);
		}
	}
}

void createClassifier()
{
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		clas[i].apfMean=0;
		clas[i].abfMean=0;
		clas[i].adfMean=0;
		clas[i].apfVar=0;
		clas[i].abfVar=0;
		clas[i].adfVar=0;
		clas[i].udp=0;
		clas[i].tcp=0;
		clas[i].icmp=0;
		clas[i].other=0;
	}
	for(int i=0;i<num_flows;i++)
	{
		clas[flows[i].attack].apfMean=flows[i].packets+clas[flows[i].attack].apfMean;
		clas[flows[i].attack].abfMean=flows[i].bytes+clas[flows[i].attack].abfMean;
		clas[flows[i].attack].adfMean=flows[i].duration+clas[flows[i].attack].adfMean;
	}
	//printf("%lf,%lf\n",num_attacks,num_normal);
	//printf("%lf,%lf,%lf\n",clas[0].apfMean,clas[0].abfMean,clas[0].adfMean);
	//printf("%lf,%lf,%lf\n",clas[1].apfMean,clas[1].abfMean,clas[1].adfMean);
	for(int i=0;i<ATTACK_TYPES;i++)
	{
	    clas[i].apfMean=clas[i].apfMean/num_attacks[i];
		clas[i].abfMean=clas[i].abfMean/num_attacks[i];
		clas[i].adfMean=clas[i].adfMean/num_attacks[i];
	}
	
	//printf("%lf,%lf,%lf\n",clas[0].apfMean,clas[0].abfMean,clas[0].adfMean);
	//printf("%lf,%lf,%lf\n",clas[1].apfMean,clas[1].abfMean,clas[1].adfMean);
	
	for(int i=0;i<num_flows;i++)
	{
		clas[flows[i].attack].apfVar=pow(flows[i].packets-clas[flows[i].attack].apfMean,2)+clas[flows[i].attack].apfVar;
		clas[flows[i].attack].abfVar=pow(flows[i].bytes-clas[flows[i].attack].abfMean,2)+clas[flows[i].attack].abfVar;
		clas[flows[i].attack].adfVar=pow(flows[i].duration-clas[flows[i].attack].adfMean,2)+clas[flows[i].attack].adfVar;		
	}

	for(int i=0;i<ATTACK_TYPES;i++)
	{
	    clas[i].apfVar=clas[i].apfVar/num_attacks[i];
		clas[i].abfVar=clas[i].abfVar/num_attacks[i];
		clas[i].adfVar=clas[i].adfVar/num_attacks[i];
		if(clas[i].apfVar==0)
			clas[i].apfVar=0.00001;
		if(clas[i].abfVar==0)
			clas[i].abfVar=0.00001;
		if(clas[i].adfVar==0)
			clas[i].adfVar=0.00001;
	}	
	
	//protocol loop - calculate P(udp/attack_type), P(tcp/attack_type), P(icmp/attack_type)
	//can be hardcoded for faster results
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		clas[i].udp=0;
		clas[i].tcp=0;
		clas[i].icmp=0;
		clas[i].other=0;
	}
	
	for(int i=0;i<num_flows;i++)
	{
		//if(stricmp(flows[i].protocol,"udp")==0)
		if(flows[i].protocol==17)
		{
			clas[flows[i].attack].udp++;
		}
		//else if(stricmp(flows[i].protocol,"tcp")==0)
		else if(flows[i].protocol==6)
		{
			clas[flows[i].attack].tcp++;
		}
		//else if(stricmp(flows[i].protocol,"icmp")==0)
		else if(flows[i].protocol==1)
		{
			clas[flows[i].attack].icmp++;
		}   
		else 
		{
			clas[flows[i].attack].other++;
		}				
	}
	
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		//printf("No of entries: udp: %Lf tcp %Lf icmp %Lf other %Lf",clas[i].udp,clas[i].tcp,clas[i].icmp,clas[i].other);
		clas[i].udp=clas[i].udp/num_attacks[i];
		clas[i].tcp=clas[i].tcp/num_attacks[i];
		clas[i].icmp=clas[i].icmp/num_attacks[i];
		clas[i].other=clas[i].other/num_attacks[i];
		//printf("No of entries: udp: %Lf tcp %Lf icmp %Lf other %Lf\n",clas[i].udp,clas[i].tcp,clas[i].icmp,clas[i].other);
		if(clas[i].udp==0)
			clas[i].udp=0.00001;
		if(clas[i].tcp==0)
			clas[i].tcp=0.00001;
		if(clas[i].icmp==0)
			clas[i].icmp=0.00001;
		if(clas[i].other==0)
			clas[i].other=0.00001;
		
		//printf("%Lf,%Lf,%Lf\n",clas[i].apfVar,clas[i].abfVar,clas[i].adfVar);
	}
	pairFlowsTraining();
	//singleFlowsTraining();
	//portsGrowthTraining();
}

//is individual flow attack or normal?
//find offending flows
int classifyFlow(int protocol,double p,double b,double d)
{
	int i=lastAttack;
	double gd1,gd2,p1,p2, p3, p4, p5, p6;
	p1=1/(sqrt(2*3.14*clas[i].apfVar));
	p2=pow(p-clas[i].apfMean,2);
	p2=-1*p2/(2*clas[i].apfVar);
	p2=exp(p2);
	
	p3=1/(sqrt(2*3.14*clas[i].abfVar));
	p4=pow(b-clas[i].abfMean,2);
	p4=-1*p4/(2*clas[i].abfVar);
	p4=exp(p4);
	
	p5=1/(sqrt(2*3.14*clas[i].adfVar));
	p6=pow(d-clas[i].adfMean,2);
	p6=-1*p6/(2*clas[i].adfVar);
	p6=exp(p6);
	
	long double pProb;	//protocol probability
	if(protocol==1)
		pProb=clas[i].icmp;
	else if(protocol==6)
		pProb=clas[i].tcp;
	else if(protocol==17)
		pProb=clas[i].udp;
	else
		pProb=clas[i].other;
	
	gd1=(p1*p2)*(p3*p4)*(p5*p6)*pProb;
	long double prob_attack=(num_attacks[i]/(double)num_flows)*gd1;	
	
	i=0;
	p1=1/(sqrt(2*3.14*clas[i].apfVar));
	p2=pow(p-clas[i].apfMean,2);
	p2=-1*p2/(2*clas[i].apfVar);
	p2=exp(p2);
	
	p3=1/(sqrt(2*3.14*clas[i].abfVar));
	p4=pow(b-clas[i].abfMean,2);
	p4=-1*p4/(2*clas[i].abfVar);
	p4=exp(p4);
	
	p5=1/(sqrt(2*3.14*clas[i].adfVar));
	p6=pow(d-clas[i].adfMean,2);
	p6=-1*p6/(2*clas[i].adfVar);
	p6=exp(p6);
	
	//long double pProb;	//protocol probability
	if(protocol==1)
		pProb=clas[i].icmp;
	else if(protocol==6)
		pProb=clas[i].tcp;
	else if(protocol==17)
		pProb=clas[i].udp;
	else
		pProb=clas[i].other;
	
	gd2=(p1*p2)*(p3*p4)*(p5*p6)*pProb;	
	long double prob_normal=(num_attacks[i]/(double)num_flows)*gd2;		
	
	if(prob_attack>prob_normal)
		return lastAttack;
	else
		return 0;
}


//void classify(int p,int b,int d)
void classify(int protocol,double p,double b,double d)
{
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		long double gd,p1,p2, p3, p4, p5, p6;
		p1=1/(sqrt(2*3.14*clas[i].apfVar));
		p2=pow(p-clas[i].apfMean,2);
		p2=-1*p2/(2*clas[i].apfVar);
		p2=exp(p2);
		
		p3=1/(sqrt(2*3.14*clas[i].abfVar));
		p4=pow(b-clas[i].abfMean,2);
		p4=-1*p4/(2*clas[i].abfVar);
		p4=exp(p4);
		
		p5=1/(sqrt(2*3.14*clas[i].adfVar));
		p6=pow(d-clas[i].adfMean,2);
		p6=-1*p6/(2*clas[i].adfVar);
		p6=exp(p6);
		
		long double pProb;	//protocol probability
		if(protocol==1)
			pProb=clas[i].icmp;
		else if(protocol==6)
			pProb=clas[i].tcp;
		else if(protocol==17)
			pProb=clas[i].udp;
		else
			pProb=clas[i].other;
			
		long double pairProb; //num of pairs probability
		int pairsRec=pairFlows();
		if(pairsRec==2)
			pairProb=clas[i].largePairClass;
		else if(pairsRec==1)
			pairProb=clas[i].mediumPairClass;
		else
			pairProb=clas[i].smallPairClass;		
		

		
		
		long double singleProb;		//single flows growth probability
		int singleRec=singleFlows();
		if(singleRec==1)
			singleProb=clas[i].largeSingleClass;
		else
			singleProb=clas[i].smallSingleClass;		
		
		
		
			
		long double portProb; //growth of ports probability
		int portsRec=portsGrowth();
		if(portsRec==1)
			portProb=clas[i].largePortClass;
		else
			portProb=clas[i].smallPortClass;		
		
		if(p4==0) p4=0.0000001;
		
		gd=(p1*p2)*(p3*p4)*(p5*p6)*pProb*pairProb;
		//gd=(p1*p2)*(p3*p4)*(p5*p6)*pProb*pairProb*portProb*singleProb;
		
		prob_attack[i]=(num_attacks[i]/(double)num_flows)*gd;
		//if(i!=4)
		//printf("%Le,%Le,%Le,%Le,%Le: %Le   %Le\n",(p1*p2),(p3*p4),(p5*p6),pProb,pairProb, p3,p4);
		printf("Attack Type %s has probability %Le\n",attacks[i],prob_attack[i]);
	}
	
	//now see who has the max probability
	long double temp=0;
	int maxIndex=0;
	int secMaxIndex=0;
	for(int i=0;i<ATTACK_TYPES;i++)
    {
        if(prob_attack[i]>temp)
        {
			temp=prob_attack[i];
			secMaxIndex=maxIndex;
			maxIndex=i;
		}
    }
	//adjustment for smurf
	if(maxIndex==2 && protocol!=1)
		maxIndex=secMaxIndex;
	//adjustment for fraggle
	if(maxIndex==3 && protocol!=17)
		maxIndex=secMaxIndex;		
	if(maxIndex!=0)
		printf("Attack detected! ");	
	printf("Classified as %s.\n",attacks[maxIndex]);
	//enter only if attack
	if(maxIndex!=0)		//any attack
	{
		//printf("Any attack!\n");
		//now classify the flows individually and get 
		if(lastAttack==-1)
		{
			lastAttack=maxIndex;
			lastAttackCounter++;
			//printf("First attack ever!\n");
		}
		else if(lastAttack==maxIndex)
		{
			lastAttackCounter++;
			//printf("Found %d\n",lastAttackCounter);
		}
		else
		{
			lastAttackCounter=0;
			lastAttack=-1;
		}
		if(lastAttackCounter==3)
		{
			printf("Deleting flows...\n");
			numOffendingFlows=0;
			for(int i=0;i<num_tflows;i++)
			{
				int attack=classifyFlow(rcv[i].protocol,rcv[i].packets,rcv[i].bytes,rcv[i].duration);
				rcv[i].attack=attack;
				if(attack!=0)
				{
					numOffendingFlows++;
					//printf("Found %d\n",numOffendingFlows);
					removeFlow(rcv[i].protocol, rcv[i].srcPort, rcv[i].dstPort, rcv[i].srcIP, rcv[i].dstIP);
				}
			}
			//printf("Offending flows:%d\n",numOffendingFlows);
			lastAttack=-1;
			lastAttackCounter=0;
		}
	}
}

void removeFlow(int protocol,int srcPort,int dstPort,char srcIP[17],char dstIP[17])
{
	char sendMsg[100], tmp[5];
	int n;
	
	sprintf(tmp, "%d", protocol);
	strcpy(sendMsg,tmp);
	strcat(sendMsg,"#");
	sprintf(tmp, "%d", srcPort);
	strcat(sendMsg,tmp);
	strcat(sendMsg,"#");
	sprintf(tmp, "%d", dstPort);
	strcat(sendMsg,tmp);
	strcat(sendMsg,"#");
	strcat(sendMsg,srcIP);
	strcat(sendMsg,"#");
	strcat(sendMsg,dstIP);
	strcat(sendMsg,"#");
	
	//printf("TO BE DELETED: %s \n", sendMsg);
	
	//if(write(s, sendMsg, sizeof(sendMsg))== -1)
	//	perror("write");
	
	if((n = sendto(sockfd, sendMsg, sizeof(sendMsg), 0, p->ai_addr, p->ai_addrlen)) == -1)
		perror("sendto");
	
	//printf("Sent %d bytes\n", n);
	
	memset(sendMsg, 0, sizeof(sendMsg));
	
}
int calculateMaxProtocol()
{
	int protocol[4];
	for(int i=0;i<4;i++)
		protocol[i]=0;
		
	for(int i=0;i<num_tflows;i++)
	{
		if(rcv[i].protocol==1) protocol[0]++;
		else if(rcv[i].protocol==6)  protocol[1]++;
		else if(rcv[i].protocol==17)  protocol[2]++;
		else protocol[3]++;
	}
	//printf("Protocol No: icmp %d, tcp %d, udp %d, other %d\n",protocol[0],protocol[1],protocol[2],protocol[3]);
	int temp;
	int max=0;
	for(int i=0;i<3;i++)
	{
		if(protocol[i+1]>protocol[i])
		{
			max=i+1;
			temp=protocol[i+1];
			protocol[i+1]=protocol[i];
			protocol[1]=temp;
		}
	}
	if(max==0)
		return 1;
	else if(max==1)
		return 6;
	else if(max==2)
		return 17;
	else 
		return 0;
}


void median(int index)
{
	int i;
	
	if (index == 0)
	{
		median_b = median_d = median_p = 0;
		return;
	}
	
	/*int bytes[MAX_FLOWS], packets[MAX_FLOWS], duration[MAX_FLOWS];					//WHY??
	for(i=0;i<index;i++)
	{
		bytes[i] = rcv[i].bytes;
		packets[i] = rcv[i].packets;
		duration[i] = rcv[i].duration;
	}
	*/
	int tmp, tmpInd;
	for(i=0;i<index-1;i++)															
	{
		if(rcv[i+1].bytes < rcv[i].bytes)
		{
			tmp = rcv[i+1].bytes;
			rcv[i+1].bytes = rcv[i].bytes;
			rcv[i].bytes = tmp;
		}
		if(rcv[i+1].packets < rcv[i].packets)
		{
			tmp = rcv[i+1].packets;
			rcv[i+1].packets = rcv[i].packets;
			rcv[i].packets = tmp;
		}
		if(rcv[i+1].duration < rcv[i].duration)
		{
			tmp = rcv[i+1].duration;
			rcv[i+1].duration = rcv[i].duration;
			rcv[i].duration = tmp;
		}
	}
	if((index+1)%2 == 0)		//even													//index taken correctly?
	{ 
		tmpInd = (index+1)/2;
		median_b = (rcv[tmpInd].bytes + rcv[tmpInd + 1].bytes) / 2;
		median_p = (rcv[tmpInd].packets + rcv[tmpInd + 1].packets) / 2;
		median_d = (rcv[tmpInd].duration + rcv[tmpInd + 1].duration) / 2;
	}
	else		//odd
	{ 
		tmpInd = ((index+1)/2) + 1;
		median_b = rcv[tmpInd].bytes;
		median_p = rcv[tmpInd].packets;
		median_d = rcv[tmpInd].duration;
	}
	//std::cout<<median_b<<std::endl<<median_p<<std::endl<<median_d<<std::endl;
}


void dCollector()
{
	int s2, index;
	unsigned int t;
    
    char str[500];

    if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
	strcpy(local.sun_path, "/users/sc558bo/Final/sock_file");
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }
	
	listen(s, 5);
    int n_bytes, n_flows, n_packets, n, num;
    printf("Waiting for a connection...\n");
    t = sizeof(remote);
	index=0;
	num_tflows=0;
	while(1)
	{
		n = read(s, str, sizeof(str));
		//printf("RCVD:%s\n",str);
		char *pch = strtok(str,"#");
		if(strcmp(pch,"END") == 0)
		{
			median(index);
			if (index !=0)
			{
				//std::cout<<index<<std::endl;
				int maxP = calculateMaxProtocol();
				//printf("Max protocol in flow table is %d\n",maxP);
				classify(maxP,median_p, median_b, median_d);
			}
			median_p = median_b = median_d = 0;
			index=0;
			num_tflows=0;
			continue;
	    }
		
		
		rcv[index].protocol=atoi(pch);
		//printf("Protocol %d\n",rcv[index].protocol);

		
		pch = strtok(NULL,"#");
		rcv[index].packets=atoi(pch);
		//std::cout<<pch<<std::endl;
		
		pch = strtok(NULL,"#");
		rcv[index].bytes = atoi(pch);
		//std::cout<<pch<<std::endl;
		
		pch = strtok(NULL,"#");
		rcv[index].duration = atoi(pch);
		//std::cout<<pch<<std::endl;

		pch = strtok(NULL,"#");
		if(strcmp(pch, "None") == 0)
			rcv[index].srcPort = 0;
		else
			rcv[index].srcPort = atoi(pch);
		
		pch = strtok(NULL,"#");
		if(strcmp(pch, "None") == 0)
			rcv[index].dstPort = 0;
		else
			rcv[index].dstPort = atoi(pch);
		//std::cout<<pch<<std::endl;
		
		pch = strtok(NULL,"#");
		strcpy(rcv[index].srcIP, pch);
		//std::cout<<pch<<std::endl;
		
		pch = strtok(NULL,"#");
		//printf("%s\n", pch);
		strcpy(rcv[index].dstIP,pch);
		
		index++;
		num_tflows++;
		//classify(rcv[index].packets, rcv[index].bytes, rcv[index].duration);
	}
}
	

int main()
{
	FILE *tFile;
	char line[1024];
	num_flows=0;
	num_normal=0;
	rec_duration=5;				//5 seconds
	lastAttack=-1;
	lastAttackCounter=0;
	for(int i=0;i<ATTACK_TYPES;i++)
	{
		num_attacks[i]=0;
		attack_duration[i]=0;
	}
		
	line[1023]='\0';
	gettimeofday(&start, NULL);
	t1=((start.tv_sec*1000000.0)+start.tv_usec)/1000;	
	tFile = fopen ("subset_training.csv" , "r");
	if (tFile == NULL) 
	{
		fprintf(stderr,"Error opening file.");
	}
	else 
	{
		while (!feof(tFile)) 
		{
			if (fgets (line , 1024 , tFile) != NULL )
			{
				validate(line);
				//printf("%s\n",line);
				num_flows++;	
			}
		}
		fclose (tFile);
	}	
	createClassifier();
	gettimeofday(&end, NULL);	
	t2=((end.tv_sec*1000000.0)+end.tv_usec)/1000;
	learningTime = t2 - t1;
	printf ("Learning time was %lf ms\n",learningTime);
	/*
	for(int i=0;i<ATTACK_TYPES;i++)
		printf("%lf, %lf\n",num_attacks[i],attack_duration[i]);

	classify(6,4,520,5);	//normal
	classify(6,6,517,1);	//normal
	classify(6,1,90,9);		//tcp syn
	classify(6,1,133,3);	//tcp syn
	classify(1,1,60,6);		//smurf
	classify(1,2,120,12);	//smurf
	classify(17,1,102,7);	//fraggle
	classify(17,1,102,0);	//fraggle
	classify(1,1,130,1);	//fraggle response
	*/
	/*if ((s2 = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, "/users/sc558bo/Final/sock_file2");
    unlink(remote.sun_path);
    len2 = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (bind(s2, (struct sockaddr *)&remote, len2) == -1) {
        perror("bind");
        exit(1);
    }
	
	listen(s2, 5);
	
	printf("Waiting for a connection...\n");
    if ((s3 = accept(s2, (struct sockaddr *)&remote, (socklen_t*)&len2)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("Connected.\n");
*/

	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo("192.168.1.102", SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }
	
	
	dCollector();
	return 0;
}

