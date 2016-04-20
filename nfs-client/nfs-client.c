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
    #include <netinet/tcp.h>
    #include <netdb.h> 
    #include "nfs-object.pb.h"

    #ifdef HAVE_SETXATTR
    #include <sys/xattr.h>
    #endif
     using namespace std;


    // nfs_proprietary function prototypes
     int sockfd;
     int  build_connection();
     int push_to_server(string);      
     void pull_from_server(char*);    



    // acutal fuse implmenetation for network based file system nfs.
     static int nfs_getattr(const char *path, struct stat *stbuf)
     {
     	int res = 0 ;
     	
     	if(build_connection() < 0) 
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
      res = send(sockfd,test.c_str(),test.length(),0); 	
      if(res < 0)
      {
         perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
         exit(0);
     }


     nfs::nfsBool nfsBool_obj;
     char buffer[BUFSIZ];
     res = recv(sockfd,buffer,BUFSIZ,0);
     if (res < 0) 
     {
            	//fprintf(stderr,"ERROR, while reading data from socket. \n");
     }

     nfsBool_obj.ParseFromString(buffer);	
     res = nfsBool_obj.result();
     if (nfsBool_obj.result() < 0) 
     {
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

    close(sockfd);
    return nfsBool_obj.result();
    }

    static int nfs_access(const char *path, int mask)
    {
    	int res;

    	if(build_connection() < 0)
    	{
    		perror("\n\tError, couldn't establish connection with server.");
    		exit(0);
    	}
    	
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
    	nfs_obj.set_methd_identfier(1);	 	
    	nfs_obj.set_mode_t(mask);
    	nfs_obj.set_item_name(nfs_path);
    	
    	string test;
    	nfs_obj.SerializeToString(&test);
    	res = send(sockfd,test.c_str(),test.length(),0); 	
    	nfs::nfsBool nfsBool_obj;
    	char buffer[BUFSIZ];
    	res = recv(sockfd,buffer,BUFSIZ,0);
    	if (res < 0) 
    	{
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
    	}

    	nfsBool_obj.ParseFromString(buffer);	
    	if (nfsBool_obj.result() < 0) 
    	{
    		close(sockfd);		
    		return nfsBool_obj.result();
    	}

    	close(sockfd);
    	return nfsBool_obj.result();
    }

    static int nfs_readlink(const char *path, char *buf, size_t size)
    {
    	
      int res;

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
    res = send(sockfd,test.c_str(),test.length(),0);  	
    if (res < 0) 
    {
     perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
     exit(0);
    }


    nfs::nfsBool nfsBool_obj;
    char buffer[BUFSIZ];
    int n = recv(sockfd,buffer,BUFSIZ,0);    
    if (n < 0) 
    {
    			//fprintf(stderr,"ERROR, while reading data from socket. \n");
    }

    nfsBool_obj.ParseFromString(buffer);
    memcpy(buf,nfsBool_obj.buffer_space().c_str(),nfsBool_obj.buffer_space().length());	 	

    if (nfsBool_obj.result() < 0)
    {
       close(sockfd);
       return nfsBool_obj.result();
    }

    close(sockfd);
    return nfsBool_obj.result();
    }


    static int nfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    	off_t offset, struct fuse_file_info *fi,
    	enum fuse_readdir_flags flags)
    {

    	if(build_connection() < 0)
    	{
    		perror("\n\tError, couldn't establish connection with server.");
    		exit(0);
    	}

    	nfs::nfsObject nfs_obj; 
    	nfs_obj.set_methd_identfier(3);	
    	
    	string nfs_path = path;	
    	nfs_obj.set_item_name(nfs_path);

    	string test;
    	nfs_obj.SerializeToString(&test);
        int res = send(sockfd,test.c_str(),test.length(),0); 	
        if(res < 0)
        {
         perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
         exit(0);
     }


     nfs::nfsDirList nfsDirlist_obj;
     char buffer[BUFSIZ];
     res = recv(sockfd,buffer,BUFSIZ,0);
     if (res < 0) 
     {
            	//fprintf(stderr,"ERROR, while reading data from socket. \n");
     }

     nfsDirlist_obj.ParseFromString(buffer);		
     if (nfsDirlist_obj.nfs_dir_result() < 0) 
     {

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


    return nfsDirlist_obj.nfs_dir_result();
    }

    static int nfs_mknod(const char *path, mode_t mode, dev_t rdev)
    {

    	int res;
    	if(build_connection() < 0)
    	{
    		perror("\n\tError, couldn't establish connection with server.");
    		exit(0);
    	}

    	
    	nfs::nfsObject nfs_obj; 
    	nfs_obj.set_methd_identfier(4);	
    	
    	string nfs_path = path;	
    	nfs_obj.set_item_name(nfs_path);
    	nfs_obj.set_mode_t(mode);
    	nfs_obj.set_st_rdev(rdev);

    	if (S_ISREG(mode)) 
    	{
    		nfs_obj.set_sys_call(0);
    	} 
    	else if (S_ISFIFO(mode))
    	{
    		nfs_obj.set_sys_call(1);
    	}
    	else
    	{
    		nfs_obj.set_sys_call(2);
    	}



    	string test;
    	nfs_obj.SerializeToString(&test); 
    	res = send(sockfd,test.c_str(),test.length(),0); 	

    	
        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        res = recv(sockfd,buffer,BUFSIZ,0);
        if (res < 0) 
        {
            	//fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

        nfsBool_obj.ParseFromString(buffer);	

        if (nfsBool_obj.result() < 0) 
        {
          close(sockfd);		
          return nfsBool_obj.result();
      }

      close(sockfd);
      return nfsBool_obj.result();

    }

    static int nfs_mkdir(const char *path, mode_t mode)
    {

      int res;
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

    res = send(sockfd,test.c_str(),test.length(),0);  	//res = mkdir(path, mode);
    if (res < 0) 
    {
       perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
       exit(0);
    }




    nfs::nfsBool nfsBool_obj;
    char buffer[BUFSIZ];
    int n = recv(sockfd,buffer,BUFSIZ,0);    
    if (n < 0) 
    {
    //fprintf(stderr,"ERROR, while reading data from socket. \n");
    }

    nfsBool_obj.ParseFromString(buffer);	 	
    if (nfsBool_obj.result() < 0)
    {
      close(sockfd);
      return nfsBool_obj.result();
    }

    close(sockfd);
    return nfsBool_obj.result();
    }

    static int nfs_unlink(const char *path)
    {
    
    int res;

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
            res = send(sockfd,test.c_str(),test.length(),0);  
            if (res < 0) 
            {
            	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }



            nfs::nfsBool nfsBool_obj;
            char buffer[BUFSIZ];
            int n = recv(sockfd,buffer,BUFSIZ,0);    
            if (n < 0) 
            {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsBool_obj.ParseFromString(buffer);	 	
            if (nfsBool_obj.result() < 0)
            {
    	
              close(sockfd);
              return nfsBool_obj.result();
          }

    	
          close(sockfd);
          return nfsBool_obj.result();
      }

      static int nfs_rmdir(const char *path)
      {
    	
         int res;


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
            res = send(sockfd,test.c_str(),test.length(),0);  //res = rmdir(path);
            if (res < 0) 
            {
            	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }



            nfs::nfsBool nfsBool_obj;
            char buffer[BUFSIZ];
            int n = recv(sockfd,buffer,BUFSIZ,0);    
            if (n < 0) 
            {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsBool_obj.ParseFromString(buffer);	 	
            if (nfsBool_obj.result() < 0)
            {
              close(sockfd);
              return nfsBool_obj.result();
          }

          close(sockfd);
          return nfsBool_obj.result();
      }

    static int nfs_symlink(const char *from, const char *to) 
    {
    	int res;
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
        res = send(sockfd,test.c_str(),test.length(),0); 
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }



        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        int n = recv(sockfd,buffer,BUFSIZ,0);    
        if (n < 0) 
        {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

        nfsBool_obj.ParseFromString(buffer);	 	
        if (nfsBool_obj.result() < 0)
        {
          close(sockfd);
          return nfsBool_obj.result();
      }

      close(sockfd);
      return nfsBool_obj.result();
    }

    static int nfs_rename(const char *from, const char *to, unsigned int flags)
    {
    	int res;	
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
        res = send(sockfd,test.c_str(),test.length(),0); 
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }



        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        int n = recv(sockfd,buffer,BUFSIZ,0);    
        if (n < 0) 
        {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

        nfsBool_obj.ParseFromString(buffer);	 	
        if (nfsBool_obj.result() < 0)
        {
          close(sockfd);
          return nfsBool_obj.result();
      }

      close(sockfd);
      return nfsBool_obj.result();
    }

    static int nfs_link(const char *from, const char *to)
    {
    	int res;	
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
        res = send(sockfd,test.c_str(),test.length(),0); 
        if (res < 0) 
        {
        	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
        	exit(0);
        }



        nfs::nfsBool nfsBool_obj;
        char buffer[BUFSIZ];
        int n = recv(sockfd,buffer,BUFSIZ,0);    
        if (n < 0) 
        {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
        }

        nfsBool_obj.ParseFromString(buffer);	 	
        if (nfsBool_obj.result() < 0)
        {
          close(sockfd);
          return nfsBool_obj.result();
      }

      close(sockfd);
      return nfsBool_obj.result();
    }

    static int nfs_chmod(const char *path, mode_t mode) //11
    {
    	int res;
    	
    	
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
            res = send(sockfd,test.c_str(),test.length(),0);  	//res = mkdir(path, mode);
            if (res < 0) 
            {
            	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }




            nfs::nfsBool nfsBool_obj;
            char buffer[BUFSIZ];
            int n = recv(sockfd,buffer,BUFSIZ,0);    
            if (n < 0) 
            {
    		  //fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsBool_obj.ParseFromString(buffer);	 	
            if (nfsBool_obj.result() < 0)
            {
              close(sockfd);
              return nfsBool_obj.result();
          }

          close(sockfd);
          return nfsBool_obj.result();
      }

      static int nfs_chown(const char *path, uid_t uid, gid_t gid)
      {
    	
         int res;
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
            res = send(sockfd,test.c_str(),test.length(),0);  	//res = mkdir(path, mode);
            if (res < 0) 
            {
            //	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }




            nfs::nfsBool nfsBool_obj;
            char buffer[BUFSIZ];
            int n = recv(sockfd,buffer,BUFSIZ,0);    
            if (n < 0) 
            {
    		  //fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsBool_obj.ParseFromString(buffer);	 	
            if (nfsBool_obj.result() < 0)
            {
              close(sockfd);
              return nfsBool_obj.result();
          }

          close(sockfd);
          return nfsBool_obj.result();
      }

      static int nfs_truncate(const char *path, off_t size)
      {
         int res;


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
            res = send(sockfd,test.c_str(),test.length(),0);  	//res = mkdir(path, mode);
            if (res < 0) 
            {
            //	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }


            nfs::nfsBool nfsBool_obj;
            char buffer[BUFSIZ];
            int n = recv(sockfd,buffer,BUFSIZ,0);    
            if (n < 0) 
            {
    		  //fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsBool_obj.ParseFromString(buffer);	 	
            if (nfsBool_obj.result() < 0)
            {
              close(sockfd);
              return nfsBool_obj.result();
          }

          close(sockfd);
          return nfsBool_obj.result();
      }

    #ifdef HAVE_UTIMENSAT
    static int nfs_utimens(const char *path, const struct timespec ts[2]) //14
    {
    	
    	int res;
    	nfs::nfsObject nfs_obj; 
    	nfs_obj.set_methd_identfier(14);

    	res = utimensat(0, path, ts, AT_SYMLINK_NOFOLLOW);
    	if (res == -1)
    		return -errno;

    	return 0;
    }
    #endif

    static int nfs_open(const char *path, struct fuse_file_info *fi)
    {
    	int res;
    	
    	
      if(build_connection() < 0)
      {
       perror("\n\tError, couldn't establish connection with server.");
       exit(0);
    }
    
    nfs::nfsObject nfs_obj; 
    nfs_obj.set_methd_identfier(15);	

    string nfs_path = path;
    nfs_obj.set_item_name(nfs_path);	
    nfs_obj.set_mode_t(fi->flags);

    string test;
    nfs_obj.SerializeToString(&test);
    res = send(sockfd,test.c_str(),test.length(),0);  	
    if (res < 0) 
    {
     perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
     exit(0);
    }

    nfs::nfsBool nfsBool_obj;
    char buffer[BUFSIZ];
    int n = recv(sockfd,buffer,BUFSIZ,0);    
    if (n < 0) 
    {
    		//fprintf(stderr,"ERROR, while reading data from socket. \n");
    }

    nfsBool_obj.ParseFromString(buffer);	 	
    if (nfsBool_obj.result() < 0)
    {
      close(sockfd);
      return nfsBool_obj.result();
    }

    fi->flags = nfsBool_obj.fi_open_flags(); 
    close(sockfd);

    return 0;
    }

    static int nfs_read(const char *path, char *buf, size_t size, off_t offset,
    	struct fuse_file_info *fi)
    {
    	//fprintf(stderr," \n ******** nfs_read ******* \n");
    	(void) fi;
    	
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
    int res = send(sockfd,test.c_str(),test.length(),0);  	
    if (res < 0) 
    {
     perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
     exit(0);
    }


    nfs::nfsBool nfsBool_obj;
    char buffer[BUFSIZ];
    int n = recv(sockfd,buffer,BUFSIZ,0);    
    if (n < 0) 
    {
    			//fprintf(stderr,"ERROR, while reading data from socket. \n");
    }

    nfsBool_obj.ParseFromString(buffer);

    if (nfsBool_obj.result() < 0)
    {
       close(sockfd);
       return nfsBool_obj.result();
    }

    if(nfsBool_obj.buffer_space().length() > 0)
    {
       memcpy(buf,(char *)nfsBool_obj.buffer_space().c_str(), nfsBool_obj.buffer_space().length());	 	
    }
    else
    {
    			//fprintf(stderr,"ERROR, memory copy error string please check \n");	
    }

    close(sockfd);
    return nfsBool_obj.result();
    }

    static int nfs_write(const char *path, const char *buf, size_t size,
    	off_t offset, struct fuse_file_info *fi)
    {
    	
    	int res;
      if(build_connection() < 0)
      {
       perror("\n\tError, couldn't establish connection with server.");
       exit(0);
    }

    nfs::nfsObject nfs_obj; 
    nfs_obj.set_methd_identfier(17);	

    string nfs_path = path;
    nfs_obj.set_item_name(nfs_path);
    string nfs_buffer = buf;

    nfs_obj.set_buffer(buf);
    nfs_obj.set_st_size(size);
    nfs_obj.set_off_set(offset);

    string test;
    nfs_obj.SerializeToString(&test);
    res = send(sockfd,test.c_str(),test.length(),0);  	
    if (res < 0) 
    {
     perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
     exit(0);
    }


    nfs::nfsBool nfsBool_obj;
    char buffer[BUFSIZ];
    int n = recv(sockfd,buffer,BUFSIZ,0);    
    if (n < 0) 
    {
    			//fprintf(stderr,"ERROR, while reading data from socket. \n");
    }

        nfsBool_obj.ParseFromString(buffer);
        if (nfsBool_obj.result() < 0)
        {
           close(sockfd);
           return nfsBool_obj.result();
        }

    		
        close(sockfd);
        return nfsBool_obj.result();
    }

    static int nfs_statfs(const char *path, struct statvfs *stbuf)
    {

        int res ;
    	if(build_connection() < 0) 
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
            res = send(sockfd,test.c_str(),test.length(),0);
            if(res < 0)
            {
            	perror("\n\tError, couldn't write onto socket.\n\n \t exiting nfs-mount-operation.");
            	exit(0);
            }


            nfs::nfsVFSStat nfsVFS_obj;
            char buffer[BUFSIZ];
            res = recv(sockfd,buffer,BUFSIZ,0);
            if (res < 0) 
            {
            	//fprintf(stderr,"ERROR, while reading data from socket. \n");
            }

            nfsVFS_obj.ParseFromString(buffer);	
            res = nfsVFS_obj.result();
            if (nfsVFS_obj.result() < 0) 
            {
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

          close(sockfd);
          return 0;
      }

      static int nfs_release(const char *path, struct fuse_file_info *fi)
      {
    	/* Just a stub.	 This method is optional and can safely be left
    	   unimplemented */
    	//fprintf(stderr," \n ******** nfs_release ******* \n");	
         (void) path;
         (void) fi;
         return 0;
     }

     static int nfs_fsync(const char *path, int isdatasync,
       struct fuse_file_info *fi)
     {
       (void) path;
       (void) isdatasync;
       (void) fi;
       return 0;
    }

    #ifdef HAVE_POSIX_FALLOCATE
    static int nfs_fallocate(const char *path, int mode,
    	off_t offset, off_t length, struct fuse_file_info *fi)
    {
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
    
    static int nfs_setxattr(const char *path, const char *name, const char *value,
    	size_t size, int flags) 0 
    {
       	int res = lsetxattr(path, name, value, size, flags);
    	if (res == -1)
    		return -errno;
    	return 0;
    }

    static int nfs_getxattr(const char *path, const char *name, char *value,
    	size_t size)
    {
       	int res = lgetxattr(path, name, value, size);
    	if (res == -1)
    		return -errno;
    	return res;
    }

    static int nfs_listxattr(const char *path, char *list, size_t size)
    {
       	int res = llistxattr(path, list, size);
    	if (res == -1)
    		return -errno;
    	return res;
    }

    static int nfs_removexattr(const char *path, const char *name)
    {
    	
    	int res = lremovexattr(path, name);
    	if (res == -1)
    		return -errno;
    	return 0;
    }

    #endif 

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

    int build_connection()
    {
    	struct hostent *sockServer;
    	struct sockaddr_in servernsock_addr;	
    	int portno = atoi("51717"); 
        int flag = 1;
    	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    	
    	setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,(char *) &flag,sizeof(int));  
    	if (sockfd < 0) 
    	{
    		return -1;
    	}

    	sockServer = gethostbyname("localhost");
    	if (sockServer == NULL) 
    	{
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
    		//fprintf(stderr,"ERROR, could not connect server.\n");
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

    	int n = send(sockfd,test.c_str(),test.length(),0);
    	if (n < 0) 
    	{
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
    		//fprintf(stderr,"\n\tERROR, while reading data from socket\n");
    	}
    	
    }

    int main(int argc, char *argv[])
    {
    	//fprintf(stderr," \n\t*** started mounting directory with nfs ***\n\n");
    	umask(0);    	

    	if(build_connection() < 0)
    	{
    		exit(0);
    	}
    	
    	initialise_nfs_operations();
    	return fuse_main(argc, argv, &nfs_oper, NULL);
    }
