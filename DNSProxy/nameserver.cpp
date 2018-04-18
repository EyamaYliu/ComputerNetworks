#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <fstream>
#include <sstream>

#include <vector>

#include "DNSHeader.h"
#include "DNSQuestion.h"
#include "DNSRecord.h"

using namespace std;

char buffer;			//using for receive data
int count = 0;		//using for count num of query

/*
	 Round robin part
*/


void header_sender(int sock, int isJHU){

	DNSHeader head;

	if(isJHU == 0){
		head.RCODE = '3';
	}else{
		head.RCODE = '0';
	}

	head.ID = 1;
	head.QR = 1;
	head.OPCODE = 1;
	head.AA = 0;
	head.TC = 1;
	head.RD = 0;
	head.RA = 0;
	head.Z = '1';

	head.QDCOUNT = 1;
	head.ANCOUNT = 1;
	head.NSCOUNT = 1;
	head.ARCOUNT = 1;

	char buffer[sizeof(head)];
	memcpy(buffer, &head, sizeof(head));
	send(sock, buffer, sizeof(buffer), 0);

	char break_point = 0x4;
	send(sock, &break_point, 1, 0);
}

void good_request_sender(int newRequest, string ipString){

	DNSRecord dnsAns;
	strncpy(dnsAns.NAME, ipString.c_str(), sizeof(dnsAns.NAME));

	dnsAns.TYPE = 1;	
	dnsAns.CLASS = 1;	
	dnsAns.TTL = 1;	
	
	char *rdata = "11";
	strcpy(dnsAns.RDATA, rdata);

	dnsAns.RDLENGTH = sizeof(dnsAns.RDATA);

	char buffer[sizeof(dnsAns)];
	memcpy(buffer, &dnsAns, sizeof(dnsAns));
	send(newRequest, buffer, sizeof(buffer), 0);

	char break_point = 0x4;
	send(newRequest, &break_point, 1, 0);
}

string request_receiver(int newRequest, char buf){
	string dns_str;
	while(true){
		int byteReceived = recv(newRequest, &buf, 1, 0);
		if(byteReceived < 0){
			perror("Receiving error");
		}
		//if proxy end send message
		if(buf == 0x4){
			break;
		}
		//still rece
		dns_str += buf;
	}
	return dns_str;
}

void roundRobin(char *log_path, int port, string servers){

	ofstream logfile;
	logfile.open(log_path);

	vector<string> ipAddr;
	ifstream ifs;
	ifs.open(servers.c_str());
	string temp;
	getline(ifs, temp);    

	while(temp != "")
	{
		stringstream ss(temp);
		ss >> temp;
		ipAddr.push_back(temp);
		getline(ifs, temp);
	}

	int dnsServer = socket(AF_INET, SOCK_STREAM, 0);
	if(dnsServer<0)
	{
		perror("Can't create socket");
		return ;
	}
	struct sockaddr_in addrDNS;
	struct sockaddr_in addrProxy;
	addrDNS.sin_family = AF_INET;
	addrDNS.sin_port = port;
	addrDNS.sin_addr.s_addr = INADDR_ANY;
	if(bind(dnsServer, (struct sockaddr *)&addrDNS, sizeof(addrDNS)) <0)
	{
		close(dnsServer);
		perror("Binding error");
		return;
	}

	if(listen(dnsServer, 10) < 0){
		close(dnsServer);
		perror("Listening error");
		return ;
	}
	socklen_t addr_size = sizeof(addrProxy);

	//in a while loop for continue listen
	while(true){
		int newRequest = accept(dnsServer, (struct sockaddr *)&addrProxy, &addr_size);
		if(newRequest<0)
		{
			close(dnsServer);
			perror("Accepting error");
			return;
		}

		//parse the client ip
		string client_ip(inet_ntoa(addrProxy.sin_addr));

		//receive dns_request
		string dns_request = request_receiver(newRequest, buffer);
		DNSHeader dnsHeader;
		memcpy(&dnsHeader, dns_request.c_str(), dns_request.size());
		if(dnsHeader.QR == 0){
			string dns_question = request_receiver(newRequest, buffer);

			DNSQuestion dnsQuestion;
			memcpy(&dnsQuestion, dns_question.c_str(), dns_question.size());


			char qname[100] = "video.cs.jhu.edu";
			if(strcmp(qname, dnsQuestion.QNAME) != 0){
				logfile << client_ip << " " << dnsQuestion.QNAME << " " << " " << endl;
				header_sender(newRequest, 0);
			}else{
				// cout<<"send good request"<<endl;
				int seq_of_server = count%ipAddr.size();
				count++;
				header_sender(newRequest, 1);
				string ipString = ipAddr[seq_of_server];
				logfile << client_ip << " " << dnsQuestion.QNAME << " " << ipString << endl;
				good_request_sender(newRequest, ipString);
			}
		}
	}
	close(dnsServer);
}





//this is main function
int main(int argc, char **argv){

	char* log_path = argv[1];
	int port = atoi(argv[2]);		
	int geography_based = atoi(argv[3]); 
	string servers(argv[4]);
	
	if(geography_based == 0){
		roundRobin(log_path,port,servers);
	}else if(geography_based == 1){
		cout << "Sorry, I ran out of time to get geography_based method implemented."<<endl;
	}else{
		printf("Error");
		return 1;
	}
}

