/*************************************************************************
	> File Name: mmap_w.c
	> Author: 
	> Mail: 
	> Created Time: 2020年04月07日 星期二 19时35分05秒
 ************************************************************************/

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <pthread.h>
struct student{
    int id;
    char name[256];
    int age;
};
void sys_err(const char* str)
{
    perror(str);
    exit(1);
}
int main(int argc,char* argv[])
{
    struct student stu;
    struct student *p;
    int fd;
    
    
    fd=open("test_map",O_RDONLY);
    if(fd==-1)
        sys_err("open error");

    ftruncate(fd,sizeof(stu));

    p=mmap(NULL,sizeof(stu),PROT_READ,MAP_SHARED,fd,0);
    if(p==MAP_FAILED)
        sys_err("mmap error");
    
    close(fd);
    while(1)
    {
        printf("id=%d,name=%s,age=%d\n",p->id,p->name,p->age);
        sleep(1);
    }
    munmap(p,sizeof(stu));




}
