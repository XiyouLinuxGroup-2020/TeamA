#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define BUF_SIZE 256
#define PROJ_ID 32
#define PATH_NAME "tmp"
#define SERVER_MSG 1
#define CLIENT_MSG 2

int main()
{
    struct mymsgbuf{
        long msgtype;
        char ctrlstring[BUF_SIZE];
    }msgbuffer;

    int qid;
    int msglen;
    key_t msgkey;

    msgkey=ftok(PATH_NAME,PROJ_ID);

    qid=msgget(msgkey,IPC_CREAT|0660);

    while(1)
    {
         msgrcv(qid,&msgbuffer,BUF_SIZE,SERVER_MSG,0);
        
        printf("server:%s\n",msgbuffer.ctrlstring);
        printf("client:");
        fgets(msgbuffer.ctrlstring,BUF_SIZE,stdin);
        if(strncmp("exit",msgbuffer.ctrlstring,4)==0)
        {
            break;
        }
        msgbuffer.ctrlstring[strlen(msgbuffer.ctrlstring)-1]='\0';
        msgbuffer.msgtype=CLIENT_MSG;

        msgsnd(qid,&msgbuffer,strlen(msgbuffer.ctrlstring)+1,0);
    }
    exit(0);
}