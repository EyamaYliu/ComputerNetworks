// iPerfer Program that designed to test connection bandwidth
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
//#define PORT 8080

int main(int argc, char const *argv[])
{

	    int serverportNum = atoi(argv[1]);   //First argument is the port number

	    int bws_fd,sev_fd, bws_socket,sev_socket, bws_read,ser_read,datasize;
	    struct sockaddr_in bwsAddress, sevAddress;//Declare two socket addresses 
	    int bwsAddrlen = sizeof(bwsAddress);
	    int sevAddrlen = sizeof(sevAddress);

	    char buffer[2000] = {0};
	    char sevbuf[20000] = {0};


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

	    sev_fd = socket(AF_INET, SOCK_STREAM, 0);
	    if (sev_fd < 0)
	    {
		perror("socket to server failed");
		exit(EXIT_FAILURE);
	    }

	    memset(&sevAddress, '0', sizeof(sevAddress));

	    //Set browser side struct address
	    sevAddress.sin_family = AF_INET;
	    sevAddress.sin_port = htons(sevPort);


	    // Convert IPv4 and IPv6 addresses from text to binary form
	    if(inet_pton(AF_INET, "10.0.0.1", &sevAddress.sin_addr)<=0) 
	    {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	    }
	  
	    if (connect(sev_fd, (struct sockaddr *)&sevAddress, sizeof(sevAddress)) < 0)
	    {
		printf("\nServer Connection Failed \n");
		return -1;
	    }


/*Send stuffs around*/

	    int respond;

	    bws_read = recv(bws_socket, buffer, sizeof(buffer),0);
	    send(sev_fd,buffer,bws_read,0);
	    //printf("%s\n",buffer);

	    //Get response from smerver
	    respond = read(sev_fd,sevbuf,sizeof(sevbuf),0);
	    send(bws_socket,sevbuf,sizeof(sevbuf),0);
	    printf("%s\n",sevbuf);



	

  	    return 0;
	    
	

}

//Use while loop from accept and connect to all the send stuff around process in order to get it working
