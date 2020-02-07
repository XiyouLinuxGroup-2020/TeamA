#include<stdio.h>
#include<time.h>
#include<sys/stat.h>
#include<unistd.h>
#include<sys/types.h>
#include<errno.h>
#include<stdlib.h>
int main(int argc,char**argv)
{
    struct stat buf;
    if(argc!=2){
        printf("usage:my_stat <filename>\n");
        exit(0);
    }
    if(stat(argv[1],&buf)==-1){ 
        perror("stat:");
        exit(0);
    }
     printf("device is:%ld\n",buf.st_dev);
     printf("inode is:%ld\n",buf.st_ino);
     printf("mode is :%o\n",buf.st_mode);
     printf("number of hard links is: %ld\n",buf.st_nlink);
     printf("user ID of owner is :%d\n",buf.st_uid);
     printf("group ID of owner is :%d\n",buf.st_gid);
     printf("device type(if inode device) is :%ld\n",buf.st_rdev);
     printf("total size in bytes is :%ld\n",buf.st_size);
     printf("blocksize for filesystem I/O is :%ld\n",buf.st_blksize);
     printf("number of blocks allocated is %ld\n",buf.st_blocks);
     printf("time of last access is :%s",ctime(&buf.st_atime));
     printf("time of last modification is :%s",ctime(&buf.st_mtime));
     printf("time of last change is :%s",ctime(&buf.st_ctime));
     return 0;
         
     
}
   