	/* A simple server in the internet domain using TCP
	   The port number is passed as an argument 
	   This version runs forever, forking off a separate 
	   process for each connection
	   Default Port # 51717
	*/

	#include <stdio.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <dirent.h>
	#include <errno.h>
	#include <sys/time.h>
	#include <stdlib.h>
	#include <string.h>
	#include <string>
	#include <sqlite3.h>
	#include <sys/types.h> 
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/statvfs.h>


	using namespace std; 

    static string nfs_path = "/root/khan/";
    static char port_no[6] = "51717";
	static int sockfd, newsockfd; //sockets to be used for incoming connections
	static socklen_t clilen;
	static struct sockaddr_in serv_addr, cli_addr; 

	// function prototypes
	int  initialise_server();  
	void read_from_socket(int); 

	// main program
	int main(int argc, char *argv[])
	{

	     initialise_server();  // intiliasing server

	     fprintf(stderr,"\n NFS-Server Started.\n");
	     int testAccept = 0;	
	     while (true)   // for never-ending loop
	     {
	     	 fprintf(stderr,"\n\t Hi Khan server waiting to accpet");	
	         newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);  // creating new socket

	         if (newsockfd < 0) 
	         {
	         	fprintf(stderr,"\n\t ERROR, while accepting connection.");
	         	exit(0);
	         }
	         else
	         {
	         	fprintf(stderr,"\n\t Hi Khan server waiting to accept");	
			testAccept = 1; 
			break;
	         }
	         
	     } 	// while ends

	     char buffer[BUFSIZ]; int first_read;
	while(true)		
	{
	     if(testAccept == 1)	
	     {
		fprintf(stderr,"\n\t inside if accept");	
		

			first_read = read(newsockfd,buffer,BUFSIZ);
			if(first_read < 0)
			{
				fprintf(stderr,"\n\t if < 0 ");					
			}

			fprintf(stderr,"\n\t inside if while %s ", buffer);	
		}
	     }

	     fprintf(stderr,"Information Message, server going to close socket.\n");   		
	     close(sockfd); //closing server

	     return 0; // never reachable statement
	 }

	// function implementations
	 int  initialise_server()
	 {
	 	fprintf(stderr,"\n initialising nfs-server ..... \n");
	 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	 	if (sockfd < 0) 
	 	{ 
	 		fprintf(stderr,"Error, Socket creation failed.\n");   
	 		return -1;
	 	}	
	 	memset((char *) &serv_addr,0, sizeof(serv_addr));	
	 	serv_addr.sin_family = AF_INET;
	 	serv_addr.sin_addr.s_addr = INADDR_ANY;
	 	serv_addr.sin_port = htons(atoi(port_no));
	 	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	 	{
	 		fprintf(stderr,"ERROR, while binding socket. \n");
	 		return -1;
	 	}     

	 	if(listen(sockfd,5) == -1 )
	 	{
	 		perror("\n\tERROR, Listening failure occured. \n");
	 		exit(0);
	 	}
	 	clilen = sizeof(cli_addr);

	 	return 0;
	 }


	 void read_from_socket(int sock)
	 {	
		
	 } // read_from_socket
