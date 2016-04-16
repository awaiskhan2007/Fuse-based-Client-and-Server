/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 * @tableofcontents
 *
 * fusexmp.c - FUSE: Filesystem in Userspace
 *
 * \section section_compile compiling this example
 *
 * gcc -Wall fusexmp.c `pkg-config fuse3 --cflags --libs` -o fusexmp
 *
 * \section section_source the complete source
 * \include fusexmp.c
 */


#define FUSE_USE_VERSION 30

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "nfs-object.pb.h"

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
 using namespace std;

// nfs_proprietary global variables
 int sockfd;
//static string nfs_path;

// nfs_proprietary function prototypes
 int  build_connection();
 int push_to_server(string);      
 void pull_from_server(char*);    
 void split_nfs_path(string,const char*);

// acutal fuse implmenetation for network based file system nfs.
 static int nfs_getattr(const char *path, struct stat *stbuf)
 {
 	int res = 0 ;
 	fprintf(stderr," \n ******** nfs_getattr %s ******* \n", path);


	if(build_connection() < 0) // reneweing socket connection
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}


	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(99);	
	
	string nfs_path = path;	
	nfs_obj.set_item_name(nfs_path);

	string test;
	nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length()); 	// res = lstat(path, stbuf); original_system_call
        if(res < 0)
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        res = read(sockfd,buffer,BUFSIZ);
        if (res < 0) 
        {
        	fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

	nfsBool_obj.ParseFromString(buffer);	// here all client-server communication completes.
	res = nfsBool_obj.result();
	if (nfsBool_obj.result() < 0) 
	{
		fprintf(stderr,"ERROR, nfs_getattr response from server: %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}
	
	stbuf->st_dev   = nfsBool_obj.nfs_stat().st_dev();
	stbuf->st_ino   = nfsBool_obj.nfs_stat().st_ino();
	stbuf->st_mode  =  nfsBool_obj.nfs_stat().st_mode();
	stbuf->st_nlink = nfsBool_obj.nfs_stat().st_nlink();
	stbuf->st_uid   = nfsBool_obj.nfs_stat().st_uid();
	stbuf->st_gid   = nfsBool_obj.nfs_stat().st_gid();
	stbuf->st_rdev	= nfsBool_obj.nfs_stat().st_rdev();
	stbuf->st_size	= nfsBool_obj.nfs_stat().st_size();
	stbuf->st_blksize = nfsBool_obj.nfs_stat().st_blksize();
	stbuf->st_blocks = nfsBool_obj.nfs_stat().st_blocks();
	stbuf->st_mtime = nfsBool_obj.nfs_stat().mtime();
	stbuf->st_ctime = nfsBool_obj.nfs_stat().ctime();
	stbuf->st_atime = nfsBool_obj.nfs_stat().atime();

	fprintf(stderr," \n ******** ending nfs_getattr %s ******* \n", path); 
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_access(const char *path, int mask)
{
	fprintf(stderr," \n ******** nfs_access %s persmissions :::: %d \n", path,mask);
	int res;

	// reneweing socket connection
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}
	// reneweing socket connection

	/* CLIENT WILL SEND REQUEST */	
	
	string nfs_path;
	if(strcmp(path,"/") == 0)
	{
		nfs_path="";	
	}
	else
	{
		nfs_path = path;	

	}

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(1);	 	// cliend filling all relevant information using object_based_message;	
	nfs_obj.set_mode_t(mask);
	nfs_obj.set_item_name(nfs_path);
	
	string test;
	nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length()); 	// res = access(path, mask); original_system_call

	/* SERVER RESPONSE AGAINST CLIENT REQUEST */	
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        res = read(sockfd,buffer,BUFSIZ);
        if (res < 0) 
        {
        	fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

	nfsBool_obj.ParseFromString(buffer);	// here all client-server communication completes.

	if (nfsBool_obj.result() < 0) 
	{
		fprintf(stderr,"ERROR, nfs_access response from server. %d \n", nfsBool_obj.result());
		close(sockfd);		
		return nfsBool_obj.result();
	}
	fprintf(stderr," \n ******** ending nfs_access %s ******* \n", path);
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_readlink(const char *path, char *buf, size_t size)
{
	fprintf(stderr," \n ******** nfs_readlink ******* \n");
	int res;
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}
		
		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(2);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_st_size(size -1 );

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
		int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
		if (n < 0) 
		{
			fprintf(stderr,"ERROR, while reading data from socket. \n");
		}

		nfsBool_obj.ParseFromString(buffer);
		memcpy(buf,nfsBool_obj.buffer_space().c_str(),nfsBool_obj.buffer_space().length());	 	

		if (nfsBool_obj.result() < 0)
		{
			fprintf(stderr,"ERROR, nfs_readlink response from server. %d \n", nfsBool_obj.result());
			close(sockfd);
			return nfsBool_obj.result();
		}

		fprintf(stderr,"\n\tvalue from server buf %s\n",nfsBool_obj.buffer_space().c_str());
		fprintf(stderr,"\n\tvalue of buf %s\n",buf);


		fprintf(stderr," \n\t ******** ending nfs_readlink ******* \n");
		close(sockfd);
		return nfsBool_obj.result();
}


static int nfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi,
	enum fuse_readdir_flags flags)
{
	fprintf(stderr," \n ******** nfs_readdir ******* \n");
	
	// reneweing socket connection
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}
	// reneweing socket connection


	// cliend filling all relevant information using object_based_message;	
	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(3);	
	
	string nfs_path = path;	
	nfs_obj.set_item_name(nfs_path);

	string test;
	nfs_obj.SerializeToString(&test);
        int res = write(sockfd,test.c_str(),test.length()); 	// res = lstat(path, stbuf); original_system_call
        if(res < 0)
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


        nfs::nfsDirList nfsDirlist_obj;
        char buffer[BUFSIZ];
        res = read(sockfd,buffer,BUFSIZ);
        if (res < 0) 
        {
        	fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

	nfsDirlist_obj.ParseFromString(buffer);		// here all client-server communication completes.
	if (nfsDirlist_obj.nfs_dir_result() < 0) 
	{
		fprintf(stderr,"ERROR, nfs_getattr response from server: %d \n", nfsDirlist_obj.nfs_dir_result());
		close(sockfd);
		return nfsDirlist_obj.nfs_dir_result();
	}

	int count = 0 ;
	while (count < nfsDirlist_obj.nfs_dir_list_size()) 
	{
		if (filler(buf, nfsDirlist_obj.nfs_dir_list(count).c_str(), NULL, 0,(enum fuse_fill_dir_flags )0))
		{
			break;
		}
		count++;
	}

	fprintf(stderr," \n ******** ending nfs_readdir %s ******* \n", path);
	return nfsDirlist_obj.nfs_dir_result();
}

static int nfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	fprintf(stderr," \n ******** nfs_mknod ******* \n");
	int res;
	
	// reneweing socket connection
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}
	// reneweing socket connection


	// cliend filling all relevant information using object_based_message;	
	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(4);	
	
	string nfs_path = path;	
	nfs_obj.set_item_name(nfs_path);
	nfs_obj.set_mode_t(mode);
	nfs_obj.set_st_rdev(rdev);


	/* On Linux this could just be 'mknod(path, mode, rdev)' but this
	   is more portable */
	if (S_ISREG(mode)) 
	{
		nfs_obj.set_sys_call(0);
		//res = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
	//	if (res >= 0)
	//	{
	//		res = close(res);
	//	}
	} 
	else if (S_ISFIFO(mode))
	{
		nfs_obj.set_sys_call(1);
		//res = mkfifo(path, mode);
	}
	else
	{
		nfs_obj.set_sys_call(2);
		//res = mknod(path, mode, rdev);
	}



	string test;
	nfs_obj.SerializeToString(&test); 
	res = write(sockfd,test.c_str(),test.length()); 	// original_system_call

	/* SERVER RESPONSE AGAINST CLIENT REQUEST */	
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        res = read(sockfd,buffer,BUFSIZ);
        if (res < 0) 
        {
        	fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

	nfsBool_obj.ParseFromString(buffer);	// here all client-server communication completes.

	if (nfsBool_obj.result() < 0) 
	{
		fprintf(stderr,"ERROR, nfs_mknode response from server. %d \n", nfsBool_obj.result());
		close(sockfd);		
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n ******** ending nfs_mknode %s ******* \n", path);
	close(sockfd);
	return nfsBool_obj.result();

	}

	static int nfs_mkdir(const char *path, mode_t mode)
	{
		fprintf(stderr," \n\t ******** nfs_mkdir path: %s ******* \n",path);
		int res;

	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}

		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(5);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_mode_t(mode);

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	//res = mkdir(path, mode);  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)

        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_mkdir response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_mkdir ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_unlink(const char *path)
{
	fprintf(stderr," \n ******** nfs_unlink ******* \n");
	int res;
	// cliend filling all relevant information using object_based_message;	
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(6);	

	string nfs_path = path;
	nfs_obj.set_item_name(nfs_path);		

	string test;
	nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  //res = rmdir(path);
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_rmdir response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_rmdir ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_rmdir(const char *path)
{
	fprintf(stderr," \n ******** nfs_rmdir ******* \n");
	int res;

	// cliend filling all relevant information using object_based_message;	
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(7);	

	string nfs_path = path;
	nfs_obj.set_item_name(nfs_path);		

	string test;
	nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  //res = rmdir(path);
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_rmdir response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_rmdir ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_symlink(const char *from, const char *to) //8
{
	fprintf(stderr," \n ******** nfs_symlink ******* \n");
	int res;
	
	// cliend filling all relevant information using object_based_message;	
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	} 

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(8);	

	string nfs_path = from;
	nfs_obj.set_item_name(nfs_path);
	nfs_obj.set_name_from(nfs_path);				
	string nfs_path_to = to;
	nfs_obj.set_name_to(nfs_path_to);		

	string test;
	nfs_obj.SerializeToString(&test);
    res = write(sockfd,test.c_str(),test.length()); 
    if (res < 0) 
    {
    	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
    	exit(0);
    }
        

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_symlink response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_symlink ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_rename(const char *from, const char *to, unsigned int flags)
{
	fprintf(stderr," \n ******** nfs_rename ******* from :: %s , to ::: %s \n",from,to);
	int res;

	// cliend filling all relevant information using object_based_message;	
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	} 

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(9);	

	string nfs_path = from;
	nfs_obj.set_item_name(nfs_path);
	nfs_obj.set_name_from(nfs_path);				
	string nfs_path_to = to;
	nfs_obj.set_name_to(nfs_path_to);		

	string test;
	nfs_obj.SerializeToString(&test);
    res = write(sockfd,test.c_str(),test.length()); 
    if (res < 0) 
    {
    	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
    	exit(0);
    }
        

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_rename response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_rename ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_link(const char *from, const char *to)
{
	fprintf(stderr," \n ******** nfs_link ******* \n");
	int res;

	// cliend filling all relevant information using object_based_message;	
	if(build_connection() < 0)
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	} 

	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(10);	

	string nfs_path = from;
	nfs_obj.set_item_name(nfs_path);
	nfs_obj.set_name_from(nfs_path);				
	string nfs_path_to = to;
	nfs_obj.set_name_to(nfs_path_to);		

	string test;
	nfs_obj.SerializeToString(&test);
    res = write(sockfd,test.c_str(),test.length()); 
    if (res < 0) 
    {
    	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
    	exit(0);
    }
        

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_link response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_link ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_chmod(const char *path, mode_t mode) //11
{
	fprintf(stderr," \n ******** nfs_chmod ******* \n");
	int res;
	
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}

		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(11);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_mode_t(mode);

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	//res = mkdir(path, mode);  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)

        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_chmod response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_chmod ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_chown(const char *path, uid_t uid, gid_t gid)
{
	fprintf(stderr," \n ******** nfs_chown ******* \n");
	int res;
	
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}

		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(12);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_st_size(uid);
		nfs_obj.set_st_rdev(gid);

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	//res = mkdir(path, mode);  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)

        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_chown response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_chown ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

static int nfs_truncate(const char *path, off_t size)
{
	fprintf(stderr," \n ******** nfs_truncate ******* \n");
	int res;
	
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}

		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(13);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_st_size(size);
	

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	//res = mkdir(path, mode);  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_chown response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fprintf(stderr," \n\t ******** ending nfs_chown ******* \n");
	close(sockfd);
	return nfsBool_obj.result();
}

#ifdef HAVE_UTIMENSAT
static int nfs_utimens(const char *path, const struct timespec ts[2]) //14
{
	fprintf(stderr," \n ******** nfs_utimens ******* \n");
	int res;
	

	// cliend filling all relevant information using object_based_message;	
	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(14);

	/* don't use utime/utimes since they follow symlinks */
	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		return -errno;

	return 0;
}
#endif

static int nfs_open(const char *path, struct fuse_file_info *fi)
{
	fprintf(stderr," \n ******** nfs_open ******* \n");
	int res;
	
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}
		fprintf(stderr," ::: fi->flags %d\n",  fi->flags);
		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(15);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_mode_t(fi->flags);

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)

        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
	int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
	if (n < 0) 
	{
		fprintf(stderr,"ERROR, while reading data from socket. \n");
	}

	nfsBool_obj.ParseFromString(buffer);	 	
	if (nfsBool_obj.result() < 0)
	{
		fprintf(stderr,"ERROR, nfs_open response from server. %d \n", nfsBool_obj.result());
		close(sockfd);
		return nfsBool_obj.result();
	}

	fi->flags = nfsBool_obj.fi_open_flags(); 
	fprintf(stderr," \n\t ******** ending nfs_open ******* \n");
	close(sockfd);

	return 0;
}

static int nfs_read(const char *path, char *buf, size_t size, off_t offset,
	struct fuse_file_info *fi)
{
	fprintf(stderr," \n ******** nfs_read ******* \n");
	(void) fi;
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}
		
		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(16);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);	
		nfs_obj.set_st_size(size);
		nfs_obj.set_off_set(offset);

		string test;
		nfs_obj.SerializeToString(&test);
        int res = write(sockfd,test.c_str(),test.length());  	  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }

	// client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
		int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
		if (n < 0) 
		{
			fprintf(stderr,"ERROR, while reading data from socket. \n");
		}

		nfsBool_obj.ParseFromString(buffer);
		//string buftest = nfsBool_obj.buffer_space().c_str();
		memcpy(buf,nfsBool_obj.buffer_space().c_str(),nfsBool_obj.buffer_space().length());	 	

		if (nfsBool_obj.result() < 0)
		{
			fprintf(stderr,"ERROR, nfs_read response from server. %d \n", nfsBool_obj.result());
			close(sockfd);
			return nfsBool_obj.result();
		}

		fprintf(stderr,"\n\tvalue from server buf %s\n",nfsBool_obj.buffer_space().c_str());
		fprintf(stderr,"\n\tvalue of buf %s\n",buf);


		fprintf(stderr," \n\t ******** ending nfs_read ******* \n");
		close(sockfd);
		return nfsBool_obj.result();
}

static int nfs_write(const char *path, const char *buf, size_t size,
	off_t offset, struct fuse_file_info *fi)
{
	fprintf(stderr," \n ******** nfs_write ******* \n");	
	int res;
	
	// cliend filling all relevant information using object_based_message;	
		if(build_connection() < 0)
		{
			perror("\n\tError, couldn't establish connection with server.");
			exit(0);
		}
		
		nfs::nfsObject nfs_obj; 
		nfs_obj.set_methd_identfier(17);	

		string nfs_path = path;
		nfs_obj.set_item_name(nfs_path);
		nfs_obj.set_name_from(string(buf, size));
		nfs_obj.set_st_size(size);
		nfs_obj.set_off_set(offset);

		string test;
		nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length());  	  // original_system_call
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }

        // client will look for server reply against the sent request. both will use the same message protocol(object_based messaging)
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
		int n = read(sockfd,buffer,BUFSIZ);    // here all client-server communication completes.
		if (n < 0) 
		{
			fprintf(stderr,"ERROR, while reading data from socket. \n");
		}

		nfsBool_obj.ParseFromString(buffer);
		if (nfsBool_obj.result() < 0)
		{
			fprintf(stderr,"ERROR, nfs_write response from server. %d \n", nfsBool_obj.result());
			close(sockfd);
			return nfsBool_obj.result();
		}

		fprintf(stderr,"\n\tvalue from server buf %s\n",nfsBool_obj.buffer_space().c_str());
		fprintf(stderr,"\n\tvalue of buf %s\n",buf);


		fprintf(stderr," \n\t ******** ending nfs_write ******* \n");
		close(sockfd);
		return nfsBool_obj.result();
}

static int nfs_statfs(const char *path, struct statvfs *stbuf)
{

 	fprintf(stderr," \n ******** nfs_statfs %s ******* \n", path);
    int res ;
	if(build_connection() < 0) // reneweing socket connection
	{
		perror("\n\tError, couldn't establish connection with server.");
		exit(0);
	}


	nfs::nfsObject nfs_obj; 
	nfs_obj.set_methd_identfier(18);	
	
	string nfs_path = path;	
	nfs_obj.set_item_name(nfs_path);

	string test;
	nfs_obj.SerializeToString(&test);
        res = write(sockfd,test.c_str(),test.length()); 	// res = lstat(path, stbuf); original_system_call
        if(res < 0)
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }


        nfs::nfsVFSStat nfsVFS_obj;
        char buffer[BUFSIZ];
        res = read(sockfd,buffer,BUFSIZ);
        if (res < 0) 
        {
        	fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

	nfsVFS_obj.ParseFromString(buffer);	// here all client-server communication completes.
	res = nfsVFS_obj.result();
	if (nfsVFS_obj.result() < 0) 
	{
		fprintf(stderr,"ERROR, nfs_getattr response from server: %d \n", nfsVFS_obj.result());
		close(sockfd);
		return nfsVFS_obj.result();
	}
	
	stbuf->f_bsize   = nfsVFS_obj.f_bsize();
	stbuf->f_frsize   = nfsVFS_obj.f_frsize();
	stbuf->f_blocks  =  nfsVFS_obj.f_blocks();
	stbuf->f_bfree = nfsVFS_obj.f_bfree();
	stbuf->f_bavail   = nfsVFS_obj.f_bavail();
	stbuf->f_files   = nfsVFS_obj.f_files();
	stbuf->f_ffree	= nfsVFS_obj.f_ffree();
	stbuf->f_favail	= nfsVFS_obj.f_favail();
	stbuf->f_fsid = nfsVFS_obj.f_fsid();
	stbuf->f_flag = nfsVFS_obj.f_flag();
	stbuf->f_namemax = nfsVFS_obj.f_namemax();
	
	fprintf(stderr," \n ******** ending nfs_getattr %s ******* \n", path); 
	close(sockfd);
	return 0;

	return 0;
}

static int nfs_release(const char *path, struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	fprintf(stderr," \n ******** nfs_release ******* \n");	
	(void) path;
	(void) fi;
	return 0;
}

static int nfs_fsync(const char *path, int isdatasync,
	struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */
	fprintf(stderr," \n ******** nfs_fsync ******* \n");	
	(void) path;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_POSIX_FALLOCATE
static int nfs_fallocate(const char *path, int mode,
	off_t offset, off_t length, struct fuse_file_info *fi)
{
	fprintf(stderr," \n ******** nfs_fallocate ******* \n");	
	int fd;
	int res;

	(void) fi;

	if (mode)
		return -EOPNOTSUPP;

	fd = open(path, O_WRONLY);
	if (fd == -1)
		return -errno;

	res = -posix_fallocate(fd, offset, length);

	close(fd);
	return res;
}
#endif

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int nfs_setxattr(const char *path, const char *name, const char *value,
	size_t size, int flags) 0 
{
	fprintf(stderr," \n ******** nfs_setxattr ******* \n");	
	int res = lsetxattr(path, name, value, size, flags);
	if (res == -1)
		return -errno;
	return 0;
}

static int nfs_getxattr(const char *path, const char *name, char *value,
	size_t size)
{
	fprintf(stderr," \n ******** nfs_getxattr ******* \n");	
	int res = lgetxattr(path, name, value, size);
	if (res == -1)
		return -errno;
	return res;
}

static int nfs_listxattr(const char *path, char *list, size_t size)
{
	fprintf(stderr," \n ******** nfs_listxattr ******* \n");	
	int res = llistxattr(path, list, size);
	if (res == -1)
		return -errno;
	return res;
}

static int nfs_removexattr(const char *path, const char *name)
{
	fprintf(stderr," \n ******** nfs_removexattr ******* \n");
	int res = lremovexattr(path, name);
	if (res == -1)
		return -errno;
	return 0;
}

#endif /* HAVE_SETXATTR */

static struct fuse_operations nfs_oper;

void initialise_nfs_operations()
{
	nfs_oper.getattr 	= nfs_getattr;
	nfs_oper.access	 	= nfs_access;
	nfs_oper.readlink	= nfs_readlink;
	nfs_oper.readdir	= nfs_readdir;
	nfs_oper.mknod		= nfs_mknod;
	nfs_oper.mkdir		= nfs_mkdir;
	nfs_oper.symlink	= nfs_symlink;
	nfs_oper.unlink		= nfs_unlink;
	nfs_oper.rmdir		= nfs_rmdir;
	nfs_oper.rename		= nfs_rename;
	nfs_oper.link		= nfs_link;
	nfs_oper.chmod		= nfs_chmod;
	nfs_oper.chown		= nfs_chown;
	nfs_oper.truncate	= nfs_truncate;
#ifdef HAVE_UTIMENSAT
	nfs_oper.utimens	= nfs_utimens;
#endif
	nfs_oper.open		= nfs_open;
	nfs_oper.read		= nfs_read;
	nfs_oper.write		= nfs_write;
	nfs_oper.statfs		= nfs_statfs;
	nfs_oper.release	= nfs_release;
	nfs_oper.fsync		= nfs_fsync;
#ifdef HAVE_POSIX_FALLOCATE
	nfs_oper.fallocate	= nfs_fallocate;
#endif
#ifdef HAVE_SETXATTR
	nfs_oper.setxattr	= nfs_setxattr;
	nfs_oper.getxattr	= nfs_getxattr;
	nfs_oper.listxattr	= nfs_listxattr;
	nfs_oper.removexattr	= nfs_removexattr;
#endif
};

/***** FUNCTION IMPLEMENTATION ***********/

int build_connection()
{
        //fprintf(stderr,"\n\t ************** SOCKET INITIALISATION *******  \n");	
	struct hostent *sockServer;
	struct sockaddr_in servernsock_addr;	
	int portno = atoi("51717"); /* binding port # */

	sockfd = socket(AF_INET, SOCK_STREAM, 0); /* opening socket */
	if (sockfd < 0) 
	{
		fprintf(stderr,"\n\t************** SOCKET OPENING ERROR *******  \n");
		return -1;
	}

	sockServer = gethostbyname("localhost");
	if (sockServer == NULL) 
	{
		fprintf(stderr,"ERROR, no such host found.\n \tnfs mount operation failed.");
		fprintf(stderr,"ERROR, exiting system.\n");
		return -1;
	}

	memset(&servernsock_addr,0,sizeof(servernsock_addr));
	servernsock_addr.sin_family = AF_INET;

	memcpy((char *)sockServer->h_addr_list[0], 
		(char *)&servernsock_addr.sin_addr.s_addr, sockServer->h_length);
	servernsock_addr.sin_port = htons(portno);

	if (connect(sockfd,(struct sockaddr *) 
		&servernsock_addr, sizeof(servernsock_addr)) < 0) 
	{
		fprintf(stderr,"ERROR, could not connect server.\n");
		return -1;
	}
	return 0;
}

int push_to_server(string hi)
{
	nfs::nfsObject nfs_obj;
	nfs_obj.set_methd_identfier(0);	
	nfs_obj.set_item_name("khan");	
	string test;
	nfs_obj.SerializeToString(&test);

	int n = write(sockfd,test.c_str(),test.length());
	if (n < 0) 
	{
		fprintf(stderr,"\n\tERROR, while writing data to socket.\n");
		return -1;
	}
	return 0;
}

void pull_from_server(char* message)
{
	memset(&message,0,sizeof(message));
	int n = read(sockfd,message,strlen(message));
	if (n < 0) 
	{
		fprintf(stderr,"\n\tERROR, while reading data from socket\n");
	}
	fprintf(stderr,"\n\t**Server-Response:  %s \n", message);
}

/* close(sockfd); need to close socket after every operation */

/**** MAIN STARTS HERE **************/

int main(int argc, char *argv[])
{
	fprintf(stderr," \n\t*** started mounting directory with nfs ***\n\n");
	umask(0);    	

	if(build_connection() < 0)
	{
		exit(0);
	}
	
	initialise_nfs_operations();
	return fuse_main(argc, argv, &nfs_oper, NULL);
}
