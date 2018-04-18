#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string>
#include <bits/stdc++.h>
#include "DNSHeader.h"
#include "DNSRecord.h"
#include "DNSQuestion.h"




using namespace std;
//#define PORT 8080
//Get header's length 
int header_length(string header)
{
	size_t fd = header.find("\r\n\r\n");
	string sub = header.substr(0, fd+4);
	return (int) sub.length();
}
//Get content length of the chunk
int content_length(string header)
{
	size_t found = header.find("Content-Length: ");
	string sub = header.substr(found+16);
	size_t fd = sub.find("\r\n");
	sub = sub.substr(0,fd);
	int length = atoi(sub.c_str());
	return length;
}
//Calculate current bitrate length
int curbitrateLeng(string header)
{	
	size_t segpos = header.find("Seg");
	string sub = header.substr(0,segpos);
	int end;
	for (int i = sub.length()-1; i >0;i--)
	{
		if(sub[i] =='/')
		{
			
			end = i;
			break;
		}
	}
	sub = sub.substr(end+1,segpos);
	int curLeng = 0;
	curLeng = sub.length();
		
	return curLeng;
}
//Find out name of requested file
string findFilename(string header)
{	
	size_t htppos = header.find("HTTP");
	size_t getpos = header.find("GET");
	string sub = header.substr(getpos+4,htppos-5);
		
	return sub;
}
//Modify header to get the bitrate of our option
string bitrateChanger(string header, int bitrate)
{
	size_t segpos = header.find("Seg");
	string sub = header.substr(0,segpos);
	string res = "";
	int end;
	for (int i = sub.length()-1; i >0;i--)
	{
		if(sub[i] =='/')
		{
			
			end = i;
			break;
		}
	}
	stringstream temp;
	temp << bitrate;
	string bit = temp.str();
	header.replace(end+1,segpos-end-1,bit);
	return header;
}
//Add _nolist to .f4m in the header
string f4mAdder(string header)
{
	size_t f4mpos = header.find(".f4m");
	string nolist = "_nolist";
	header.insert(f4mpos,nolist);
	
	return header;
}


string GetDNSip(int dnsPort,char* dnsIp,char*domain)
{

    DNSHeader request;

    int sock = socket(AF_INET, SOCK_STREAM,0);
    if(sock == -1)
    {
        perror("socket failed");
        return "";
    }
    struct sockaddr_in addrDNSserver;
    addrDNSserver.sin_family = AF_INET;
	addrDNSserver.sin_addr.s_addr = inet_addr(dnsIp);
    addrDNSserver.sin_port = dnsPort;

    if(connect(sock,(struct sockaddr *)&addrDNSserver,sizeof(addrDNSserver)) < 0)
    {
		printf("DNS Connection Failed \n");
		return "";
    }

    char buff[sizeof(request)];
    memcpy(buff, &request,sizeof(request));    
    send(sock,buff,sizeof(buff),0);

    char breakPoint = 0x4;
    send(sock, &breakPoint, 1,0);




    DNSQuestion question;

    strncpy(question.QNAME, domain, sizeof(question.QNAME));
    char buff2[sizeof(question)];
    memcpy(buff2, &question,sizeof(question));
    send(sock,buff2,sizeof(buff2),0);

    char buf;
    string dnsStr;
    while(true)
    {
        int received = recv(sock, &buf,1,0);
        if (received <0){ perror("Receiving error");}
        if(buf == 0x4){break;} //If proxy is closed
        dnsStr += buf;
    }

    DNSHeader dnsHdr;
    memcpy(&dnsHdr,dnsStr.c_str(),dnsStr.size());
    if(dnsHdr.RCODE == '0')
    {
        string dnsStr;
        while(true)
        {
            int received = recv(sock,&buf,1,0);
            if (received <0){ perror("Receiving error");}
            if(buf == 0x4){break;} //If proxy is closed
            dnsStr+=buf;
        }
        DNSRecord dnsAns;
        memcpy(&dnsAns,dnsStr.c_str(),dnsStr.size());
        cout << "The IP is: "<<dnsAns.NAME<<endl;
        return string(dnsAns.NAME);
    }else
    {
        return "";
    }



}



int main(int argc, char *argv[])
{
	if (argc < 6)
	{
		cout <<"Error:Missing arguments."<<endl;
		exit(1);
	}else if(argc > 7)
    {
        cout <<"Error: Additional arguments."<<endl;
        exit(1);
    }
	int serverportNum = atoi(argv[3]);   //First argument is the port number
	int bws_fd,sev_fd, bws_socket,sev_socket, len,ser_read,datasize;
	struct sockaddr_in bwsAddress, sevAddress;//Declare two socket addresses 
	int bwsAddrlen = sizeof(bwsAddress);
	int sevAddrlen = sizeof(sevAddress);
	
	char buffer[1000] = {0};
	char sevbuf[1000] = {0};
	char sevbuf_2[1000] = {0};
	char md_rq[1000];
	char dns_ip[20];
	strcpy(dns_ip,argv[4]);
    int dns_port = atoi(argv[5]);
    char www_ip[20];
    strcpy(www_ip,"video.cs.jhu.edu");
    if (argc == 7)
    {
        strcpy(www_ip,argv[6]);
    }
        

	float alpha = atof(argv[2]);
    
	if(alpha >1 || alpha<0)
	{
		cout <<"Alpha value out of limit, should be in [0,1]"<<endl;
		exit(1);
	}
/*Setting log_file*/
	char *logname = argv[1];
	string log = logname;
	ofstream logfile;
	logfile.open(logname);
	logfile<<"<Duration(s)> <tput(Mbps)> <avg-tput(Mbps)> <bitrate(Kbps)> <server-ip> <chunkname>"<<endl;
	
/*Initializing socket file descriptors for browser*/
	// Creating socket file descriptor for browser
	bws_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (bws_fd == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	//Set browser side struct address
	bwsAddress.sin_family = AF_INET;
	bwsAddress.sin_addr.s_addr = INADDR_ANY;
	bwsAddress.sin_port = htons( serverportNum );
	//bind and listen for browser
	if (bind(bws_fd, (struct sockaddr *)&bwsAddress,sizeof(bwsAddress))<0)
	{
		perror("browser side bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(bws_fd, 3) < 0)
	{
		perror("browser side listen error");
		exit(EXIT_FAILURE);
	}
	//accept bws's header
	if ((bws_socket = accept(bws_fd, (struct sockaddr *)&bwsAddress,(socklen_t*)&bwsAddrlen))<0)
	{
		perror("browser side accept error");
		exit(EXIT_FAILURE);
	}
/*Initializing socket file descriptors for server*/
	int sevPort = 80;

    const char* sev_adr;
    string temp = GetDNSip(dns_port,dns_ip,www_ip);
    if(temp == "") 
    {
        sev_adr = www_ip;
    }else
    {
        sev_adr =temp.c_str();
    }

	sev_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sev_fd < 0)
	{
		perror("socket to server failed");
		exit(EXIT_FAILURE);
	}
	memset(&sevAddress, '0', sizeof(sevAddress));
	//Set server side struct address
	sevAddress.sin_family = AF_INET;
	sevAddress.sin_port = htons(sevPort);
    sevAddress.sin_addr.s_addr = inet_addr(sev_adr);
	// Convert IPv4 and IPv6 addresses from text to binary form

/*	if(inet_pton(AF_INET, dns_IP, &sevAddress.sin_addr)<=0) 
	{
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}*/
	  
	if (connect(sev_fd, (struct sockaddr *)&sevAddress, sizeof(sevAddress)) < 0)
	{
		printf("\nServer Connection Failed \n");
		return -1;
	}
/*Send stuffs around*/
	int respond = 1;
	int i = 0;
//	clock_t start;
	struct timeval start,end;
	double elapsed;//Time difference for receive chunks
	double throughput;
	double t1,t2; //Timers
	double T_cur = 0; //Throughput calculation
	double allowedRate = 0;
	int btrlistlen = 0;
	string filename;

	 //Initiate bitrate options and declare size of the array
	int bitrateLists[] =  {10,100,500,1000};
	btrlistlen = sizeof(bitrateLists)/sizeof(bitrateLists[0]);

	while(true)
	{
		int chunksize = 0;
		//Start to count time
		gettimeofday(&start,NULL);
		//Read from browser
		len = recv(bws_socket, buffer, sizeof(buffer),0);
		string str_req = buffer; //Assign browser's request to variable req in string format
		//If find .f4m, add _nolist and increase length of browser request by length of _nolist
		bool no_list = false;
		int curRateLeng = 0;
		int btrToUse;

		if(str_req.find(".f4m") != string::npos)
		{
			cout<<"Found f4m!"<<endl;
			str_req = f4mAdder(str_req);
			cout << "_nolist is added!" <<endl;
			len = len + 7;
			no_list = true;
		}
		if(str_req.find("Seg") != string::npos)
		{
			//Check
			//To determine which bitrate is good to be use under current situation
			btrToUse = bitrateLists[0];
			for (int i = 0; i < btrlistlen; i++) 
			{
				if(bitrateLists[i] < allowedRate)
				{
					btrToUse = bitrateLists[i];
				}
			}
			//Check the length of the current Rate for len to be modified
			curRateLeng = curbitrateLeng(str_req);
			//Replace the current Rate to choosen rate	
			str_req = bitrateChanger(str_req,btrToUse);
			stringstream temp;
			temp << btrToUse;
			string bit = temp.str();
			//Modify len to be the actuall length after replace the bitrate
			len = len + bit.length()-curRateLeng;
			filename = findFilename(str_req);
			
		}
		strcpy(md_rq,str_req.c_str());
		//Send modified request to server
		send(sev_fd,md_rq,len,0);
		//Get response from server, get headerlength and file length
		respond = recv(sev_fd,sevbuf,sizeof(sevbuf),0);
		if (respond == 0)
		{
			shutdown(sev_fd,2);
			connect(sev_fd, (struct sockaddr *)&sevAddress, sizeof(sevAddress));
		}
		else if (respond < 0)
		{
			perror("recv");
		}
		chunksize += respond;
		string header = "";
		header = sevbuf;
		int headerlen = header_length(header);
		int contentlen = content_length(header);
		int remain = contentlen + headerlen - respond;
		
		send(bws_socket,sevbuf,respond,0);
	
		while(remain != 0)
		{
			respond = recv(sev_fd,sevbuf_2,sizeof(sevbuf_2),0);
			remain = remain - respond;
			send(bws_socket,sevbuf_2,respond,0);
			chunksize += respond;
		}
		gettimeofday(&end,NULL);
		//End of the time count
		t2 = (double)(end.tv_usec)/1000000 + (double)(end.tv_sec);
		t1 = (double)(start.tv_usec)/1000000 + (double)(start.tv_sec);
		elapsed = t2-t1; //Calculate timer
		//Calculate throughput
		throughput = chunksize * 8 /(elapsed*1000000);
		T_cur = alpha * throughput + (1-alpha) * T_cur;
		
		allowedRate = 1000*T_cur / 1.5;
		
		
		
		logfile <<elapsed<<" "<<throughput<<" "<<T_cur<<" "<<btrToUse<<" "<<sev_adr<<" "<<filename<<endl;
		
	   }
	logfile.close();
	
  	return 0;
}
	
