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
	#include "nfs-object.pb.h"

	using namespace std; 

    static string nfs_path = "/root/khan/";
    static char port_no[6] = "51717";
	static int sockfd, newsockfd; //sockets to be used for incoming connections
	static socklen_t clilen;
	static struct sockaddr_in serv_addr, cli_addr; 

	// function prototypes
	int  initialise_server();  
	void read_from_socket(int); 

	struct timespecs
	{
		int tv_sec;
		long  tv_nsec; 
	} ts[2];

	// main program
	int main(int argc, char *argv[])
	{

	     initialise_server();  // intiliasing server

	     fprintf(stderr,"\n NFS-Server Started.\n");

	     int pid; //int con_accpt_sucess;
	     while (true)   // for never-ending loop
	     {
	         newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);  // creating new socket

	         if (newsockfd < 0) 
	         {
	         	fprintf(stderr,"\n\t ERROR, while accepting connection.");
	         	exit(0);
	         }

	       //  con_accpt_sucess = 1 ;
	         
	         pid = fork(); // generating new process id for incoming requests

	         if (pid < 0)
	         {
	         	fprintf(stderr,"\n\tERROR, couldn't generate process.");
	         	exit(0);
	         } 

	         if (pid == 0)  
	         {
	             close(sockfd); //closing existing socket file descriptor table
	             read_from_socket(newsockfd);  // processing client request
	             exit(0);
	         }
	         else 
	         { 
				close(newsockfd); //closing socket
			 }	

	     } 		// while ends

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

	   char buffer[BUFSIZ];  
	   nfs::nfsObject nfs_obj;  //  send and receive object-based message passing protocol  
	   int first_read = read(sock,buffer,BUFSIZ);
	   if (first_read < 0)
	   {
	   	fprintf(stderr,"ERROR, while reading data from first_read socket. %d \n",first_read);
	   	exit(0);
	   }
	   nfs_obj.ParseFromString(buffer);

	   switch(nfs_obj.methd_identfier())	//handle all fuse-related system calls here via switch-case statements
	   {	

	   	case 0:
	   	{
	   		fprintf(stderr,"\n\t client left the mount point. \n");
	   	}
	   	break;

	   	case 99:
	   	{

	   		nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0;    struct stat stbuf;	
	   		string dir_name = nfs_path + nfs_obj.item_name().c_str();
	   		fprintf(stderr,"\n case 99 .... \n");
	   		fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = lstat(dir_name.c_str(), &stbuf); 
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing lstat. %d :::: %d\n", sys_res, errno);	    
				sys_res = -errno;
			}

			nfsBool_obj.mutable_nfs_stat()->set_st_dev(stbuf.st_dev);
			nfsBool_obj.mutable_nfs_stat()->set_st_ino(stbuf.st_ino);
			nfsBool_obj.mutable_nfs_stat()->set_st_mode(stbuf.st_mode);
			nfsBool_obj.mutable_nfs_stat()->set_st_nlink(stbuf.st_nlink);
			nfsBool_obj.mutable_nfs_stat()->set_st_uid(stbuf.st_uid);
			nfsBool_obj.mutable_nfs_stat()->set_st_gid(stbuf.st_gid);
			nfsBool_obj.mutable_nfs_stat()->set_st_rdev(stbuf.st_rdev);
			nfsBool_obj.mutable_nfs_stat()->set_st_size(stbuf.st_size);
			nfsBool_obj.mutable_nfs_stat()->set_st_blksize(stbuf.st_blksize);
			nfsBool_obj.mutable_nfs_stat()->set_st_blocks(stbuf.st_blocks);
			nfsBool_obj.mutable_nfs_stat()->set_mtime(stbuf.st_mtime);
			nfsBool_obj.mutable_nfs_stat()->set_ctime(stbuf.st_ctime);
			nfsBool_obj.mutable_nfs_stat()->set_atime(stbuf.st_atime);
			
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"ERROR, while responding to client ");
			} 
		}
		break;

		case 1:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 1 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = access(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 2:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; char buf[BUFSIZ];
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 2 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = readlink(dir_name.c_str(),buf,nfs_obj.st_size());   
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call open. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.set_buffer_space(string(buf,sys_res));
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call open. %d  \n", sock_res);
			}
		}
		break;

		case 3:    
		{
			fprintf(stderr,"\n case 3 .... \n");
			nfs::nfsDirList nfsDirlist_obj; struct dirent *de;  string nfs_result;
			int sock_res = 0; int sys_res;
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n server starting requested operation ..... %s. \n", dir_name.c_str());
			DIR *dp = opendir(dir_name.c_str());

			if (dp == NULL)
			{
				sys_res = -errno;
				fprintf(stderr,"\n\t Error, directory not found ..... %d. \n", -sys_res);
			}
			
			nfsDirlist_obj.set_nfs_dir_result(sys_res);
			while ((de = readdir(dp)) != NULL) 
			{
				nfsDirlist_obj.add_nfs_dir_list(de->d_name);
				//fprintf(stderr,"\n server starting requested operation ..... %s. \n", de->d_name);
			}
			closedir(dp);

			nfsDirlist_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;
		
		case 4:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 4 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  

			if(nfs_obj.sys_call() == 0)
			{
				fprintf(stderr,"\n case 4 .... regular file case\n");	
				sys_res = open(dir_name.c_str(), O_CREAT | O_EXCL | O_WRONLY, nfs_obj.mode_t());
				if(sys_res >= 0)
				{
					sys_res = close(sys_res);
				}
			}
			else if (nfs_obj.sys_call() == 1)
			{
				fprintf(stderr,"\n case 4 .... S_ISFIFO case\n");	
				sys_res = mkfifo(dir_name.c_str(), nfs_obj.mode_t());
			}
			else 
			{
				fprintf(stderr,"\n case 4 .... else-case\n");
				sys_res = mknod(dir_name.c_str(), nfs_obj.mode_t(), nfs_obj.st_rdev());
			}

			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing mknode. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;
		
		case 5:
		{
			
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 5 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = mkdir(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call mkdir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call mkdir. %d  \n", sock_res);
			}  
		}
		break;
		
		case 6:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 6 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = unlink(dir_name.c_str());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call rmdir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call rmdir. %d  \n", sock_res);
			}

		}
		break;

		case 7:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 7 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = rmdir(dir_name.c_str());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call rmdir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call rmdir. %d  \n", sock_res);
			}  
			
		}
		break;

		case 8:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			fprintf(stderr,"\n case 8 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name_from.c_str());	  
			sys_res = symlink(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call renamedir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  
		}
		break;

		case 9:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			fprintf(stderr,"\n case 9 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name_from.c_str());	  
			sys_res = rename(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call renamedir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  

		}
		break;

		case 10:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			fprintf(stderr,"\n case 10 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name_from.c_str());	  
			sys_res = link(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call renamedir. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  
		}
		break;

		case 11:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 11 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = chmod(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 12:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 12 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = lchown(dir_name.c_str(),nfs_obj.st_size(), nfs_obj.st_rdev());
			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 13:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 13 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = truncate(dir_name.c_str(),nfs_obj.st_size());
			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 14:
		{
			/*nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 14 .... \n");	
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  

			sys_res = utimensat(0,dir_name.c_str(),nfs_obj.st_size()); //utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
			if (sys_res < 0)
			{
				fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client ");
			} 
			*/
		}
		break;

		case 15:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 15 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = open(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call open. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.set_fi_open_flags(nfs_obj.mode_t());	
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"\n\t ERROR, while responding to client after system call open. %d  \n", sock_res);
			}  
		}
		break;

		case 16:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; int fd;
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 16 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = open(dir_name.c_str(), O_RDONLY);
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call nfs_read. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
				if (sock_res < 0) 
				{
					fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_read. %d  \n", sock_res);
				}
			}
			else
			{
				fd = sys_res; char buf[BUFSIZ]; int size = nfs_obj.st_size(); int offset = nfs_obj.off_set();
				fprintf(stderr,"\n\t attributes sent from  client in nfs_read. %lld :::: %lld  \n", nfs_obj.off_set(),nfs_obj.st_size());	
				sys_res = pread(fd, buf, size, offset);
				
				fprintf(stderr,"\n\t attributes values after system in nfs_read. %lld :::: %lld  \n", nfsBool_obj.st_offset(),nfsBool_obj.st_size());	
				fprintf(stderr,"\n\t P-read returns the string  %s  \n", buf);

				if(sys_res<0){
					//error
				}
				else{
					nfsBool_obj.set_result(sys_res);
					nfsBool_obj.set_buffer_space(string(buf,sys_res));
					nfsBool_obj.SerializeToString(&nfs_result);
					sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
				}
				
				if (sock_res < 0) 
				{
					fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_read. %d  \n", sock_res);
				}
				close(fd);
			} 
		}
		break;

		case 17:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; int fd;
			string dir_name = nfs_path + nfs_obj.item_name();
			fprintf(stderr,"\n case 17 .... \n");
			fprintf(stderr,"\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = open(dir_name.c_str(), O_WRONLY);
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing system call nfs_write. %d :::: %d \n", sys_res,errno);      
				sys_res = -errno ; 
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
				if (sock_res < 0) 
				{
					fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_write. %d  \n", sock_res);
				}
			}
			else
			{
				fd = sys_res;  int size = nfs_obj.st_size(); int offset = nfs_obj.off_set();
				fprintf(stderr,"\n\t attributes sent from  client in nfs_write. %lld :::: %lld  \n", nfs_obj.off_set(),nfs_obj.st_size());	
				sys_res = pwrite(fd, nfs_obj.name_from().c_str(), size, offset);
				fprintf(stderr,"\n\t attributes values after system in nfs_write. %lld :::: %lld  \n", nfsBool_obj.st_offset(),nfsBool_obj.st_size());	
				
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
				if (sock_res < 0) 
				{
					fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_write. %d  \n", sock_res);
				}
				close(fd);	
			}
		}
		break;

		case 18:
		{
			nfs::nfsVFSStat nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0;    struct statvfs stbuf;	
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			fprintf(stderr,"\n case 18 .... \n");
			fprintf(stderr,"\n\n\t server starting requested operation ..... %s \n", dir_name.c_str());	  
			sys_res = statvfs(dir_name.c_str(), &stbuf); // processing specific system call
			if (sys_res < 0)
			{
				fprintf(stderr,"\n\t ERROR, while executing lstat. %d :::: %d\n", sys_res, errno);	    
				sys_res = -errno;
			}

			// responding to client according to system call result
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.set_f_bsize(stbuf.f_bsize);
			nfsBool_obj.set_f_frsize(stbuf.f_frsize);
			nfsBool_obj.set_f_blocks(stbuf.f_blocks);
			nfsBool_obj.set_f_bfree(stbuf.f_bfree);
			nfsBool_obj.set_f_bavail(stbuf.f_bavail);
			nfsBool_obj.set_f_files(stbuf.f_files);
			nfsBool_obj.set_f_ffree(stbuf.f_ffree);
			nfsBool_obj.set_f_favail(stbuf.f_favail);
			nfsBool_obj.set_f_fsid(stbuf.f_fsid);
			nfsBool_obj.set_f_flag(stbuf.f_flag);
			nfsBool_obj.set_f_namemax(stbuf.f_namemax);

			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = write(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				fprintf(stderr,"ERROR, while responding to client ");
			} 
		}
		break;

		default:
		fprintf(stderr,"\n default called.... \n");

		}	//fuse-handler via switch ends here

	} // read_from_socket
