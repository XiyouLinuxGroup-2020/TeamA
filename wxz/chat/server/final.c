#include "final.h"

int main()
{
    int sys_log;


    Read_from_mysql();

    printf("线程池启动\n");
    pool_init(MAX_THREAD_NUM);
    printf("线程池启动成功!\n");
    sleep(2);


    Init_socket();

    Connect_mysql();


    pool_destroy();

}
void Init_socket()
{
    //初始化链表list。链表为带头结点的双向循环链表
    List_Init(list_ser,server_user_node_t);
  
    PACK recv_t;
    printf("服务端启动\n");
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    socklen_t cli_addr_len;

    lfd=Socket(AF_INET,SOCK_STREAM,0);

    //端口复用
    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));

    bzero(&serv_addr,sizeof(serv_addr));

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    Bind(lfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    Listen(lfd,128);

    printf("服务器启动成功!\n");

    epfd=epoll_create(1024);
    struct epoll_event tep,ep[1024];
    tep.events=EPOLLIN;
    tep.data.fd=lfd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&tep);

    int i;
    int ret;
    while(1)
    {
        ret=epoll_wait(epfd,ep,1024,-1);
        for(i=0;i<ret;i++)
        {
            if (!(ep[i].events & EPOLLIN))      //如果不是"读"事件, 继续循环
                continue;

            if(ep[i].data.fd==lfd)
            {
                cli_addr_len=sizeof(cli_addr);
                cfd=Accept(lfd,(struct sockaddr*)&cli_addr,&cli_addr_len);
                printf("连接到新的客户端ip:%s\n端口号:%d\n",inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);
                tep.events=EPOLLIN;
                tep.data.fd=cfd;
                epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&tep);
            }
            else if(ep[i].events & EPOLLIN)
            {
                int n=recv(ep[i].data.fd,&recv_t,sizeof(PACK),0);
                recv_t.data.send_fd=ep[i].data.fd;

                Recv_pack_message(recv_t);

                if(n<0)
                {
                    close(ep[i].data.fd);
                    sys_err("接收错误!",__LINE__);
                    continue;
                }
                else if(n==0)
                {
                    server_list_t pos;
                    //使用指针pos依次遍历链表list_ser
                    List_ForEach(list_ser,pos)
                    {
                        if(pos->data.socket_id==ep[i].data.fd)
                        {
                            List_FreeNode(pos);
                            printf("%s downline\n",pos->data.username);
                            break;
                        }
                    }
                    printf("客户端连接断开\n");
                    tep.data.fd=ep[i].data.fd;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,ep[i].data.fd,&tep);
                    close(ep[i].data.fd);
                    continue;
                }

                PACK *recv_pack_t;
                recv_pack_t=(PACK*)malloc(sizeof(PACK));
                memcpy(recv_pack_t,&recv_t,sizeof(PACK));

                threadpool_add(work,(void*)recv_pack_t);

               
            }
            
        }
    }

    close(epfd);
    close(lfd);
}

void *work(void* arg)
{
    PACK* pack_t;
    pack_t=(PACK*)arg;

    switch(pack_t->flag)
    {
        case LOGIN:
            Login(pack_t);
            break;
        case REGISTER:
            Register(pack_t);
            break;
        case ADD_FRIEND:
            Add_friend(pack_t);
            break;
        case DEL_FRIEND:
            Del_friend(pack_t);
            break;
        case QUERY_FRIEND:
            Query_friend(pack_t);
            break;
        case PRIVATE_CHAT:
            Private_chat(pack_t);
            break;
        //写在一起比较好
        case SHOW_FRIEND_STATUS:
        case VIEW_FRIEND_LIST:
            Show_friend_status(pack_t);
            break;
        case VIEW_CHAT_HISTORY:
            View_chat_history(pack_t);
            break;
        case SHIELD:
            Shield_friend(pack_t);
            break;
        case UNSHIELD:
            Unshield_friend(pack_t);
            break;
        case CREAT_GROUP:
            Create_group(pack_t);
            break;
        case ADD_GROUP:
            Add_group(pack_t);
            break;
        case WITHDRAW_GROUP:
            Withdraw_group(pack_t);
            break;
        case VIEW_ADD_GROUP:
            View_add_group(pack_t);
            break;
        case VIEW_GROUP_MEMBER:
            View_group_member(pack_t);
            break;        
        case VIEW_GROUP_RECORD:
            View_group_record(pack_t);
            break;
        case DEL_GROUP:
            Del_group(pack_t);
            break;
        case SET_GROUP_ADMIN:
            Set_group_admin(pack_t);
            break;
        case KICK:
            Kick(pack_t);
            break;
        case SEND_FILE:
            Send_file(pack_t);
            break;
        case 0:
            break;
    }
    return NULL;
}
void Register(PACK* pack_t)
{
    char register_flag[10];
    int flag;
    char buf[MAX];

    server_list_t pos;
    for(pos=list_ser->next;pos!=list_ser;pos=pos->next)
    {
        if(strcmp(pos->data.username,pack_t->data.send_name)==0)
        {
            printf("pos:%s\n%s\n",pos->data.username,pos->data.password);
            flag=1;
            break;
        }
    }

    server_user_node_t *new;
    new=(server_user_node_t*)malloc(sizeof(server_user_node_t));

    if(flag==0)
    {
        strcpy(new->data.username,pack_t->data.send_name);
        strcpy(new->data.password,pack_t->data.message);
        new->data.online=DOWNLINE;
        
        //链表尾插法，list为头指针，new为新节点
        List_AddTail(list_ser,new);
        user_num++;


        memset(buf,0,sizeof(buf));
        sprintf(buf,"insert into account values('%s','%s')",pack_t->data.send_name,pack_t->data.message);
        mysql_real_query(&mysql,buf,strlen(buf));
        register_flag[0]='1';
    }
    else
    {
        register_flag[0]='0';
    }
    register_flag[1]='\0';

    Send_recv_pack(pack_t->data.send_fd,pack_t,register_flag);
    free(pack_t);
}
//登录
void Login(PACK* pack_t)
{
    char login_flag[10];
    int ret;
    int flag=0;
    
    
    server_list_t pos;
    for(pos=list_ser->next;pos!=list_ser;pos=pos->next)
    {
        if((strcmp(pos->data.username,pack_t->data.send_name)==0)&&strcmp(pos->data.password,pack_t->data.message)==0)
        {
            printf("pos:%s\n%s\n",pos->data.username,pos->data.password);
            flag=1;
            
            break;
        }
    }

    //账号已经登录
    if(flag==1)
    {
        if(pos->data.online==ONLINE)
            login_flag[0]='3';
    }

    //密码不正确
    if(flag==0)
        login_flag[0]='0';
    else
    {
        if(pos->data.online==DOWNLINE)
        {
            //登录成功
            login_flag[0]='1';
            pos->data.online=ONLINE;
            pos->data.connfd=pack_t->data.send_fd;
        }
        else
        {
            //账号不存在
            login_flag[0]='2';
        }
    }
    login_flag[1]='\0';

    Send_recv_pack(pack_t->data.send_fd,pack_t,login_flag);
    
    free(pack_t);
}

void Add_friend(PACK* pack_t)
{
    server_list_t pos;
    server_list_t pos_friend;
    pos=Find_server_user(pack_t->data.send_name);
    pos_friend=Find_server_user(pack_t->data.message);

    strcpy(pos->data.friend_message[ (pos->data.friend_num) ++],pack_t->data.message);
    //strcpy(pos->data.friend_message[ (pos->data.friend_num) ++],pos_friend->data.username);
    strcpy(pos_friend->data.friend_message[ (pos_friend->data.friend_num) ++],pack_t->data.send_name);
    //strcpy(pos_friend->data.friend_message[ (pos_friend->data.friend_num) ++],pos->data.username);
    
    free(pack_t);
}
void Del_friend(PACK* pack_t)
{
    server_list_t pos;
    server_list_t pos_friend;

    pos=Find_server_user(pack_t->data.send_name);
    Find_del_server_user(pos,pack_t->data.message);

    pos_friend=Find_server_user(pack_t->data.message);
    Find_del_server_user(pos_friend,pack_t->data.send_name);

    free(pack_t);
}
/*void Query_friend(PACK* pack_t)
{
    
}*/
void Private_chat(PACK* pack_t)
{
    Mysql_save_message(pack_t);
    Send_pack(pack_t);
    free(pack_t);
}
/*void Shield_friend(PACK* pack_t);
void Unshield_friend(PACK* pack_t);*/
void Show_friend_status(PACK* pack_t)
{
    char status[MAX_CHAR*2];
    char buf[MAX_CHAR];
    char name_t[MAX_CHAR];
    int cnt;

    memset(status,0,sizeof(status));
    server_list_t pos;
    pos=Find_server_user(pack_t->data.send_name);
    
    status[cnt++]=pos->data.friend_num;
    
    int i,j,k;
    for(i=1;i<=pos->data.friend_num;i++)
    {
        strcpy(name_t,pos->data.friend_message[i]);
        for(j=1;j<=user_num;j++)
        {
            if(strcmp(name_t,pos->data.username)==0)
            {
                memset(buf,0,sizeof(buf));
                if(pos->data.online==ONLINE)
                    sprintf(buf,"[%s]:[%d]",pos->data.username,ONLINE);
                else
                {
                    sprintf(buf,"[%s]:[%d]",pos->data.username,DOWNLINE);
                }
                printf("buf=%s\n",buf);
                for(k=0;k<SAVE;k++)
                    status[k+cnt]=buf[k];
                cnt+=SAVE;
            }
        }
    }

    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");
    memcpy(pack_t->data.message,status,sizeof(status));
    pack_t->data.recv_fd=pack_t->data.send_fd;
    pack_t->data.send_fd=lfd;

    Send_pack(pack_t);
    free(pack_t);
}

/*void View_friend_list(PACK* pack_t);*/
//void View_chat_history(PACK* pack_t);
void Create_group(PACK* pack_t)
{

    
    int flag;
    group_list_t pos;

    pos=Find_server_group(pack_t->data.message);

    if(pos!=NULL)
    {
        strcpy(pack_t->data.recv_name,pack_t->data.send_name);
        strcpy(pack_t->data.send_name,"server");
        pack_t->data.message[0]=1;
        Send_pack(pack_t);
        free(pack_t);
        return ;
    }

    //新建群信息
    group_node_t* new;
    new=(group_node_t*)malloc(sizeof(group_node_t));
    new->data.member_num=0;
    //群名
    strcpy(new->data.group_name,pack_t->data.message);
    //群主
    strcpy(new->data.member_name[new->data.member_num],pack_t->data.send_name);
    new->data.type[new->data.member_num]=1;
    new->data.member_num=1;

    server_user_node_t* new_user;
    new_user=Find_server_user(pack_t->data.send_name);
    strcpy(new_user->data.group[new_user->data.group_num++].group_name,pack_t->data.message);

    //链表尾插法，list为头指针，new为新节点
    List_AddTail(group_ser,new);
    group_num++;

    printf("\ngroup:%s create !\n",pack_t->data.message);

    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");

    pack_t->data.message[0]=2;
    Send_pack(pack_t);
    free(pack_t);
}
void Add_group(PACK* pack_t)
{
    group_list_t pos;
    pos=Find_server_group(pack_t->data.message);
    if(pos!=NULL)
    {
        strcpy(pos->data.member_name[pos->data.member_num],pack_t->data.send_name);
        pos->data.type[pos->data.member_num]=3;
        pos->data.status[pos->data.member_num]=1;
        pos->data.member_num++;


        strcpy(pack_t->data.recv_name,pack_t->data.send_name);
        strcpy(pack_t->data.send_name,"server");

        pack_t->data.message[0]=2;

        printf("\nuser:%s add group:%s successfully !\n",pack_t->data.recv_name,pack_t->data.message);

        Send_pack(pack_t);
        free(pack_t);
        return ;
    }
    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");

    pack_t->data.message[0]=1;
    Send_pack(pack_t);
    free(pack_t);
}
void Withdraw_group(PACK* pack_t)
{
    int i,j,k;
    group_list_t pos;
    pos=Find_server_group(pack_t->data.message);
    
    for(i=1;i<=group_num;i++)
    {
        if(strcmp(pack_t->data.message,pos->data.group_name)==0)
        {
            for(j=1;j<=pos->data.member_num;j++)
            {
                if(strcmp(pack_t->data.send_name,pos->data.member_name[j])==0)
                {
                    for(k=j;k<pos->data.member_num;k++)
                    {
                        strcpy(pos->data.member_name[k],pos->data.member_name[k+1]);
                        pos->data.type[k]=pos->data.type[k+1];
                        pos->data.status[k]=pos->data.status[k+1];
                    }
                    pos->data.member_num--;
                }
            }
        }
    }
}
void View_add_group(PACK* pack_t)
{
    char status[MAX_CHAR*2];
    char buf[MAX_CHAR];
    char name_t[MAX_CHAR];
    int cnt;

    memset(status,0,sizeof(status));
    server_list_t pos;
    pos=Find_server_user(pack_t->data.send_name);
    
    
    status[cnt++]=pos->data.group_num;
    int i,j,k;
    for(i=1;i<=pos->data.group_num;i++)
    {
        memset(buf,0,sizeof(buf));
        strcpy(name_t,pos->data.group_message[i]);
        sprintf(buf,"[%s]",name_t);
        for(j=0;j<SAVE;j++)
            status[j+cnt]=buf[j];
        cnt+=SAVE;
    }



    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");
    memcpy(pack_t->data.message,status,sizeof(status));
    pack_t->data.recv_fd=pack_t->data.send_fd;
    pack_t->data.send_fd=lfd;

    Send_pack(pack_t);
    free(pack_t);
}
void View_group_member(PACK* pack_t)
{
    char status[MAX_CHAR*2];
    int type_t;
    int status_t;
    char buf[MAX_CHAR];
    char name_t[MAX_CHAR];
    int cnt;

    memset(status,0,sizeof(status));
    group_list_t pos;
    pos=Find_server_group(pack_t->data.message);

    status[cnt++]=pos->data.member_num;

    int i,j,k;


    for(i=1;i<=pos->data.member_num;i++)
    {
        memset(buf,0,sizeof(buf));
        strcpy(name_t,pos->data.member_name[i]);
        type_t=pos->data.type[i];
        status_t=pos->data.status[i];
        sprintf(buf,"[%s]:[权限:%d]([%d])",name_t,type_t,status_t);
        for(j=0;j<SAVE;j++)
            status[j+cnt]=buf[j];
        cnt+=SAVE;
    }


    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");
    memcpy(pack_t->data.message,status,sizeof(status));
    pack_t->data.recv_fd=pack_t->data.send_fd;
    pack_t->data.send_fd=lfd;

    Send_pack(pack_t);
    free(pack_t);

}
/*void View_group_record(PACK* pack_t);*/
void Group_chat(PACK* pack_t)
{
    group_list_t pos;
    pos=Find_server_group(pack_t->data.recv_name);
    char send_name[SAVE];

    int i;
    int len=strlen(pack_t->data.message);
    for(i=len;i>=0;i--)
    {
        pack_t->data.message[i+SAVE]=pack_t->data.message[i];
    }
    //把客户端消息copy
    strcpy(send_name,pack_t->data.send_name);

    for(i=0;i<SAVE;i++)
    {
        pack_t->data.message[i]=pack_t->data.send_name[i];
    }
    strcpy(pack_t->data.send_name,pack_t->data.recv_name);

    for(i=1;i<=pos->data.member_num;i++)
    {
        strcpy(pack_t->data.recv_name,pos->data.member_name[i]);
        if(strcmp(send_name,pos->data.member_name[i])!=0)
        {
            Mysql_save_message(pack_t);

            Send_pack(pack_t);
        }
    }
    free(pack_t);

}


void Del_group(PACK* pack_t)
{
    int i;
    int j;
    group_list_t pos=Find_server_group(pack_t->data.message);
    for(i=1;i<=group_num;i++)
    {
        if(strcmp(pos->data.group_name,pack_t->data.message)==0)
        {
           if(strcmp(pos->data.member_name[1],pack_t->data.send_name)==0)
           {
               //从链表中删除并释放结点node
               //
               List_FreeNode(pos);
               pack_t->data.message[0]=2;
           }
           else
           {
               pack_t->data.message[0]=1;
           }
           
        }
    }
    strcpy(pack_t->data.recv_name,pack_t->data.send_name);
    strcpy(pack_t->data.send_name,"server");

 
    Send_pack(pack_t);
    free(pack_t);
}
/*void Set_group_admin(PACK* pack_t);
void Kick(PACK* pack_t);*/
void my_err(const char* err_string,int line)
{
    fprintf(stderr,"line:%d",line);
    perror(err_string);
    exit(1);
}
void Clear_buffer()
{
    char ch;
    while(getchar()!='\n')
		continue;
    /*while((ch=getchar())!='\n' && ch!=EOF)
            continue;*/
}
void display(char* str)
{
    int i;
    system("clear");
    for(i=0;i<50;i++)
        putchar('-');
    putchar('\n');
    printf("       %s\n",str);
    for(i=0;i<50;i++)
        putchar('-');
    putchar('\n');
    return;
}
char getch()
{
	char ch;

    system("stty -echo");//不回显
    system("stty -icanon");//设置一次性读完操作，如使用getchar()读操作,不需要按回车
    ch = getchar();
    system("stty icanon");//取消上面的设置
    system("stty echo");//回显

    return ch;
}
char* Get_string(char* buf,int len)
{
    char* str;
    int i=0;
    str=fgets(buf,len,stdin);
	if(str!=NULL)
	{
		while(str[i]!='\0' && str[i]!='\n')
			i++;
		if(str[i]=='\n')
			str[i]='\0';
		else
			while(getchar()!='\n')
				continue;
	}
	return str;
}
void Recv_pack_message(PACK recv_t)
{
    printf("\n\033[1;33m--------PACK--------\033[0m\n");
    printf("\033[1;33m|\033[0m 类型  :\n");
    printf("\033[1;33m|\033[0m 发送包的名字  :%s\n",recv_t.data.send_name);
    printf("\033[1;33m|\033[0m 接受包的名字  :%s\n",recv_t.data.recv_name);
    printf("\033[1;33m|\033[0m 信息  :%s\n",recv_t.data.message);
    printf("\033[1;33m|\033[0m 发送者fd  :%d\n",recv_t.data.send_fd);
    printf("\033[1;33m|\033[0m 接收者fd  :%d\n",recv_t.data.recv_fd);
    printf("\033[1;33m|\033[0m 发送包的数量:%d\n",send_num);
    printf("\033[1;33m---------------------\033[0m\n");
}
void Send_recv_pack(int fd,PACK* recv_pack,char* flag)
{
    PACK pack_send;
    
    memcpy(&pack_send,recv_pack,sizeof(PACK));
    printf("%s\n%s\n",pack_send.data.recv_name,pack_send.data.send_name);

    strcpy(pack_send.data.recv_name,pack_send.data.send_name);
    strcpy(pack_send.data.send_name,"server");
    strcpy(pack_send.data.message,flag);
    printf("%s\n%s\n",pack_send.data.recv_name,pack_send.data.send_name);

    printf("%s\n",pack_send.data.message);
    pack_send.data.recv_fd=pack_send.data.send_fd;
    pack_send.data.send_fd=fd;

    if(send(fd,&pack_send,sizeof(PACK),0)<0)
    {
        my_err("send error!",__LINE__);
    }
}
void Send_pack(PACK* send_pack_t)
{
    pthread_mutex_lock(&mutex);
    memcpy(&(pack_send[send_num++]),send_pack_t,sizeof(PACK));
    pthread_mutex_unlock(&mutex);
}
server_list_t Find_server_user(char *username)
{
    if(user_num==0)
        return NULL;
    
    server_list_t pos=list_ser;
    if(pos==NULL)
        return NULL;
    for(pos=list_ser->next;pos!=list_ser;pos=pos->next)
    {
        if((strcmp(pos->data.username,username)==0))
        {
            printf("pos:%s\n%s\n",pos->data.username,pos->data.password);
            return pos;
        }
    }
    return NULL;
}
void Find_del_server_user(server_list_t pos,char* friend_name)
{
    int i;
    int index;
    for(pos=list_ser->next,i=0;pos!=list_ser;pos=pos->next,i++)
    {
        if((strcmp(pos->data.friend_message[i],friend_name)==0))
        {
            printf("pos:%s\n%s\n",pos->data.username,pos->data.password);
            index=i;
        }
    }
    for(pos=list_ser->next,i=index;pos!=list_ser;pos=pos->next,i++)
    {
        strcpy(pos->data.friend_message[i],pos->data.friend_message[i+1]);
    }
    pos->data.friend_num--;
}

group_list_t Find_server_group(char* group_name)
{
    group_list_t pos=group_ser;
    if(group_num==0)
        return NULL;
    if(pos==NULL)
        return NULL;
    for(pos=group_ser->next;pos!=group_ser;pos=pos->next)
    {
        if((strcmp(pos->data.group_name,group_name)==0))
        {
            printf("pos:%s\n",pos->data.group_name);
            return pos;
        }
    }
    return NULL;
}
/*
void Read_from_mysql()
{

    //初始化链表list。链表为带头结点的双向循环链表
    List_Init(list_ser,server_list_t);
    List_Init(group_ser,group_list_t);

    


}*/
