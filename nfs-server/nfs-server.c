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
    #include <netinet/tcp.h>
	#include "nfs-object.pb.h"

	using namespace std; 

    static string nfs_path = "/root/khan";
    static char port_no[6] = "51717";
	static int sockfd, newsockfd; 
	static socklen_t clilen;
	static struct sockaddr_in serv_addr, cli_addr; 

	// function prototypes
	int  initialise_server();  
	void read_from_socket(int); 


	int main(int argc, char *argv[])
	{

	     initialise_server();  // intiliasing server

	     fprintf(stderr,"\n NFS-Server Started.\n");

	     int pid; 
	     while (true)   
	     {

	         newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen); 

	         if (newsockfd < 0) 
	         {
	         	fprintf(stderr,"\n\t ERROR, while accepting connection.");
	         	exit(0);
	         }
	         
	         pid = fork(); 

	         if (pid < 0)
	         {
	         	//fprintf(stderr,"\n\tERROR, couldn't generate process.");
	         	exit(0);
	         } 

	         if (pid == 0)  
	         {
	             close(sockfd); 
	             read_from_socket(newsockfd);
	             exit(0);
	         }
	         else 
	         { 
				close(newsockfd); 
			 }	

	     } 		


	     close(sockfd); 
	     return 0;
	 }

	// function implementations
	 int  initialise_server()
	 {
	 	//fprintf(stderr,"\n initialising nfs-server ..... \n");
	 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	 	int flag = 1;
		setsockopt(sockfd,            /* socket affected */
                        IPPROTO_TCP,     /* set option at TCP level */
                        TCP_NODELAY,     /* name of option */
                        (char *) &flag,  /* the cast is historical cruft */
                        sizeof(int));    /* length of option value */
	 	if (sockfd < 0) 
	 	{ 
	 		//fprintf(stderr,"Error, Socket creation failed.\n");   
	 		return -1;
	 	}	

	 	memset((char *) &serv_addr,0, sizeof(serv_addr));	
	 	serv_addr.sin_family = AF_INET;
	 	serv_addr.sin_addr.s_addr = INADDR_ANY;
	 	serv_addr.sin_port = htons(atoi(port_no));
	 	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	 	{
	 		fprintf(stderr,"ERROR, while binding socket. please re-start nfs-server\n");
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
	   nfs::nfsObject nfs_obj;  
	   int first_read = recv(sock,buffer,BUFSIZ,0);
	   if (first_read < 0)
	   {
	   		exit(0);
	   }
	   
	   nfs_obj.ParseFromString(buffer);

	   switch(nfs_obj.methd_identfier())	
	   {	

	   	case 0:
	   	{
	   		//fprintf(stderr,"\n\t client left the mount point. \n");
	   	}
	   	break;

	   	case 99:
	   	{

	   		nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0;    struct stat stbuf;	
	   		string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = lstat(dir_name.c_str(), &stbuf); 
			if (sys_res < 0)
			{
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
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"ERROR, while responding to client ");
			} 
		}
		break;

		case 1:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = access(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 2:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; char buf[BUFSIZ];
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = readlink(dir_name.c_str(),buf,nfs_obj.st_size());   
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.set_buffer_space(string(buf,sys_res));
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 3:    
		{
			nfs::nfsDirList nfsDirlist_obj; struct dirent *de;  string nfs_result;
			int sock_res = 0; int sys_res;
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			DIR *dp = opendir(dir_name.c_str());

			if (dp == NULL)
			{
				sys_res = -errno;
			}
			
			nfsDirlist_obj.set_nfs_dir_result(sys_res);
			while ((de = readdir(dp)) != NULL) 
			{
				nfsDirlist_obj.add_nfs_dir_list(de->d_name);
			}
			closedir(dp);

			nfsDirlist_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;
		
		case 4:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			
			if(nfs_obj.sys_call() == 0)
			{
				sys_res = open(dir_name.c_str(), O_CREAT | O_EXCL | O_WRONLY, nfs_obj.mode_t());
				if(sys_res >= 0)
				{
					sys_res = close(sys_res);
				}
			}
			else if (nfs_obj.sys_call() == 1)
			{
				sys_res = mkfifo(dir_name.c_str(), nfs_obj.mode_t());
			}
			else 
			{
				sys_res = mknod(dir_name.c_str(), nfs_obj.mode_t(), nfs_obj.st_rdev());
			}

			if (sys_res < 0)
			{
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;
		
		case 5:
		{
			
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = mkdir(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call mkdir. %d  \n", sock_res);
			}  
		}
		break;
		
		case 6:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = unlink(dir_name.c_str());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call rmdir. %d  \n", sock_res);
			}

		}
		break;

		case 7:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = rmdir(dir_name.c_str());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call rmdir. %d  \n", sock_res);
			}  
			
		}
		break;

		case 8:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			sys_res = symlink(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  
		}
		break;

		case 9:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			sys_res = rename(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  

		}
		break;

		case 10:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name_from = nfs_path + nfs_obj.name_from().c_str();
			string dir_name_to = nfs_path + nfs_obj.name_to().c_str();
			sys_res = link(dir_name_from.c_str(),dir_name_to.c_str());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call renamedir. %d  \n", sock_res);
			}  
		}
		break;

		case 11:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = chmod(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 12:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = lchown(dir_name.c_str(),nfs_obj.st_size(), nfs_obj.st_rdev());
			if (sys_res < 0)
			{
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 13:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = truncate(dir_name.c_str(),nfs_obj.st_size());
			if (sys_res < 0)
			{
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			}
		}
		break;

		case 14:
		{
			/*nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = utimensat(0,dir_name.c_str(),nfs_obj.st_size()); //utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
			if (sys_res < 0)
			{
				//fprintf(stderr,"ERROR, while executing access. %d \n", sys_res);
				sys_res = -errno;	        
			}

			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length());
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client ");
			} 
			*/
		}
		break;

		case 15:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; 
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = open(dir_name.c_str(),nfs_obj.mode_t());
			if (sys_res < 0)
			{
				sys_res = -errno ; 
			} 
			nfsBool_obj.set_result(sys_res);
			nfsBool_obj.set_fi_open_flags(nfs_obj.mode_t());	
			nfsBool_obj.SerializeToString(&nfs_result);
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"\n\t ERROR, while responding to client after system call open. %d  \n", sock_res);
			}  
		}
		break;

		case 16:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; int fd;
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			sys_res = open(dir_name.c_str(), O_RDONLY);
			if (sys_res < 0)
			{
				sys_res = -errno ; 
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
				if (sock_res < 0) 
				{
					//fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_read. %d  \n", sock_res);
				}
			}
			else
			{
				fd = sys_res; char buf[BUFSIZ]; int size = nfs_obj.st_size(); int offset = nfs_obj.off_set();
				sys_res = pread(fd, buf, size, offset);		
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.set_buffer_space(buf);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
				if (sock_res < 0) 
				{
					//fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_read. %d  \n", sock_res);
				}
				close(fd);
			} 
		}
		break;

		case 17:
		{
			nfs::nfsBool nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0; int fd;
			string dir_name = nfs_path + nfs_obj.item_name();
			sys_res = open(dir_name.c_str(), O_WRONLY);
			if (sys_res < 0)
			{
				sys_res = -errno ; 
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
				if (sock_res < 0) 
				{
					//fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_write. %d  \n", sock_res);
				}
			}
			else
			{
				fd = sys_res;  int size = nfs_obj.st_size(); int offset = nfs_obj.off_set();
				sys_res = pwrite(fd, nfs_obj.buffer().c_str(), size, offset);
				nfsBool_obj.set_result(sys_res);
				nfsBool_obj.SerializeToString(&nfs_result);
				sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
				if (sock_res < 0) 
				{
					//fprintf(stderr,"\n\t ERROR, while responding to client after system call nfs_write. %d  \n", sock_res);
				}
				close(fd);	
			}
		}
		break;

		case 18:
		{
			nfs::nfsVFSStat nfsBool_obj; int sys_res; string nfs_result; int sock_res = 0;    struct statvfs stbuf;	
			string dir_name = nfs_path + nfs_obj.item_name().c_str();
			
			sys_res = statvfs(dir_name.c_str(), &stbuf); 
			if (sys_res < 0)
			{
				//fprintf(stderr,"\n\t ERROR, while executing lstat. %d :::: %d\n", sys_res, errno);	    
				sys_res = -errno;
			}

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
			sock_res = send(sock,nfs_result.c_str(),nfs_result.length(),0);
			if (sock_res < 0) 
			{
				//fprintf(stderr,"ERROR, while responding to client ");
			} 
		}
		break;

		default:
		fprintf(stderr,"\n default called.... \n");

		}	//fuse-handler via switch ends here

	} // read_from_socket