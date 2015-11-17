/******************************************************************************
  Copyright (c) 2015 by Baida.zhang(Tong.zhang)  - zt9788@126.com

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "pushserver.h"
#include "log.h"
#include "md5.h"
//#include "uuid32.h"
#include "readconf.h"

#include "util.h"
#include "thread_pool.h"

#include "daemon.h"
#include <sys/resource.h>
#include <syslog.h>
#include <time.h>

#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif

#ifdef DEBUG
//#undef Log
//#define Log(format,...) NULL
//#else
#define DEBUGFMT  "File:%s(%d)-%s:"
#define DEBUGARGS __FILE__,__LINE__,__FUNCTION__
//#define Log(format, ...) Log (DEBUGFMT format, ##__VA_ARGS__)
#define __DEBUG(format, ...) printf("FILE: "__FILE__", LINE: %d: "format"/n", __LINE__, ##__VA_ARGS__)
#define _freeReplyObject(rd)\
	printf("FILE: "__FILE__", LINE: %d: \n /n",__LINE__ ); \
	freeReplyObject(rd);

// printf (DEBUGFMT format,DEBUGARGS, ##__VA_ARGS__)
//#define __DEBUG( ) printf (DEBUGFMT,DEBUGARGS)
#endif


#define SEM_NAME "mysem"
#define OPEN_FLAG O_RDWR|O_CREAT
#define OPEN_MODE 00777
#define INIT_V    1


static struct config_struct system_config;

static pthread_mutex_t lock;

//static sem_t **sem = NULL;
static pid_t pid = -1;
pid_t main_pid = -1;
//static volatile sig_atomic_t _running = 1;
volatile int g_isExit = 0;
volatile int g_isRestart = 0;
#ifndef __EPOLL__
unsigned int lisnum = FD_SETSIZE;
CLIENT g_clients[FD_SETSIZE];

#else
unsigned int lisnum = MAX_CONNECTION_LENGTH;
CLIENT g_clients[MAX_CONNECTION_LENGTH];
static struct epoll_event eventList[MAX_EVENTS];
#endif
static pthread_mutex_t client_lock;
pthread_t* coretthreadsId;
int active_socket_connect = 0;
int g_maxi = -1;
static int g_epollfd;
fd_set rset,allset;


void* read_thread_function(void* client_t);
void* unionPushList(void* st);
void* pushlist_send_thread(void* st);//thread_function
/*
//信号捕捉处理
static void myhandler(char* name,sem_t* semt)
{
    //删掉在系统创建的信号量
    sem_unlink(name);
    //彻底销毁打开的信号量
    sem_close(semt);
}
*/
int addepollevent(int fd,void* ptr)
{
#ifdef __EPOLL__
    struct epoll_event event;
    if(ptr == NULL)
        event.data.fd = fd;
    else
        event.data.ptr = ptr;
    event.events =  EPOLLIN|EPOLLET;  //EPOLLET
    if(epoll_ctl(g_epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        Log("epoll add fail : fd = %d\n", fd);
        return -1;
    }
#endif
    return 0;
}

int mksock(int type)
{
    int sock;
    struct sockaddr_in  my_addr;
    if((sock = socket(AF_INET,type,0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    //bzero(&my_addr,sizeof(my_addr));
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(system_config.server_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //close socket just now
    unsigned int value = 0x1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void *)&value,sizeof(value));
    return sock;

}

/***************************
**server for multi-client
**PF_SETSIZE=1024
****************************/
//#define FD_SETSIZE 1024
int main(int argc, char** argv)
{

    int i,n;
    int nready;
    //int autoclientid = 1;
    int slisten,sockfd,maxfd=-1;//,connectfd;

    //unsigned int lisnum;

    struct sockaddr_in  my_addr,addr;
    struct timeval tv;

    char buf[MAXBUF + 1];

    if(argc < 2)
    {
        Log("Usage: %s configfile",argv[0]);
        exit(0);
    }
    read_config(argv[1],&system_config);
    print_welcome();
    if(strcasecmp(system_config.deamon,"yes") == 0)
    {
        init_daemon(argv[1]);
    }

    slisten = mksock(SOCK_STREAM);

//    bzero(&my_addr,sizeof(my_addr));
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(system_config.server_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    pthread_mutex_init(&client_lock, NULL);
    if(bind(slisten, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(slisten, lisnum) == -1)
    {
        perror("listen");
        exit(1);
    }
    pthread_mutex_lock(&client_lock);
#ifndef __EPOLL__
    for(i=0; i<FD_SETSIZE; i++)
#else
    for(i=0; i<MAX_CONNECTION_LENGTH; i++)
#endif
    {
        g_clients[i].fd = -1;
        g_clients[i].clientId = i;
        g_clients[i].serverid = system_config.serverid;
        memset(g_clients[i].drivceId,0,64);
    }
    pthread_mutex_unlock(&client_lock);

    FD_ZERO(&allset);
    FD_SET(slisten, &allset);
    maxfd = slisten;

    Log("Waiting for connections and data...\n");

    //start push quene
    struct pushstruct st;
    st.client = g_clients;
    st.timeout=system_config.push_time_out;

    struct pushstruct st2;
    memcpy(&st2,&st,sizeof(pushstruct));
    st2.timeout=system_config.delay_time_out;

    createRedisPool(system_config.redis_ip, system_config.redis_port,30);
    int redisID= -1;
    redisContext* redis = getRedis(&redisID);
    tpool_create(system_config.max_recv_thread_pool);
    pthread_mutex_init(&st.lock, NULL);
    pthread_mutex_lock(&st.lock);
    st.isexit = 0;
    int ij=0;
    coretthreadsId = malloc(system_config.push_thread_count+system_config.unin_thread_count);
    for(ij=0; ij<system_config.push_thread_count; ij++)
    {
        //tpool_add_work(pushlist_send_thread,(void*)&st);
        pthread_create(&coretthreadsId[ij],NULL,pushlist_send_thread,(void*)&st);
    }
    pthread_mutex_unlock(&st.lock);
    pthread_mutex_init(&st2.lock, NULL);
    pthread_mutex_lock(&st2.lock);
    st2.isexit = 0;
    //tpool_add_work(unionPushList,(void*)&st2);
    pthread_create(&coretthreadsId[ij],NULL,unionPushList,(void*)&st2);
    pthread_mutex_unlock(&st2.lock);
    //end

    //create processer

    int processercount = system_config.processer;
    //pid_t* fid = malloc(sizeof(pid_t)*processercount);
    //sem = malloc(sizeof(sem_t)* processercount);
    char tempsemStr[10];
    itoa_(0,tempsemStr);
    //sem[0] = sem_open(tempsemStr, OPEN_FLAG, OPEN_MODE, INIT_V);
    main_pid = getpid();
    /*************************************
    //多fork 一个要解决惊群问题，还有一个就是client数组轮训的时候，不知道是那个
    //fork接收的数据
    for(ij=0; ij<processercount; ij++)
    {
    	fid[ij] = fork();
    	if(fid[ij]<0)
    	{
    		printf("create process error");
    	}
    	//sun process
    	if(fid[ij] == 0)
    	{
    		pid = getpid();//fid[ij];
    		itoa_(pid,tempsemStr);
    		sem[ij+1] = sem_open(tempsemStr, OPEN_FLAG, OPEN_MODE, INIT_V);
    		break;
    	}
    	//father process
    	else
    	{
    		pid = getpid();//fid[ij];
    	}
    }

    ********************************************/


    Log("this process's pid=%d , and main pid is %d\n",pid,main_pid);
#ifdef __EPOLL__
    //1. epoll 初始化
    int epollfd = epoll_create(MAX_EVENTS);
    g_epollfd = epollfd;
    if(addepollevent(slisten,NULL)<0)
    {
        Log("epoll add fail : fd = %d\n", slisten);
        return -1;
    }
    //EPOLL相关
    //事件数组
    if(pid != 0)
    {
        addepollevent(0,NULL);
    }

#endif

    while (g_isExit == 0 && g_isRestart == 0)//sig = xxx wait for sig
    {
        rset = allset;
        if(pid == main_pid)
        {
            FD_SET(0, &rset);
        }

        pthread_mutex_lock(&client_lock);
#ifdef __EPOLL__
        int timeout = 5;
        nready = epoll_wait(epollfd, eventList, MAX_EVENTS, timeout);
#else
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        nready = select(maxfd + 1, &rset, NULL, NULL, &tv);
#endif
        pthread_mutex_unlock(&client_lock);
        int isQuit = 0;
        if(nready == 0)
        {
            //printf("timeout ...\n");
            continue;
        }
        else if(nready < 0)
        {
            printf("select failed!\n");
            break;
        }
        else
        {
#ifdef __EPOLL__
            int n = 0;

            for(n=0; n<nready; n++)
            {
                //错误退出
                if ((eventList[n].events & EPOLLERR) ||
                        (eventList[n].events & EPOLLHUP) ||
                        !(eventList[n].events & EPOLLIN))
                {
                    Log ( "epoll error\n");
                    CLIENT* clientx = (CLIENT*)eventList[n].data.ptr;
                    if(eventList[n].data.ptr < 0) continue;
                    //int sockfd = -1;
                    if((sockfd = clientx->fd)<0)
                        continue;
                    //removeClientInfoFromRedis(redis,clientx);
                    removeClient(clientx);//TODO here some time client is not have drivceid may make a mistake
                    Log("eventList error！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                    //epoll_ctl(epollfd,  EPOLL_CTL_DEL, sockfd, NULL);
                    //close_socket(sockfd,g_clients,clientx->clientId,&allset,epollfd);
                    close_socket(clientx,&allset);
                    //close (fd);
//					return -1;
                    continue;
                }
                else  if (eventList[n].data.fd == slisten)
                {

                    int ret = new_connection(slisten,&addr,g_clients,&allset,&maxfd,epollfd);
                    if(ret != 1)
                        continue;
                }
                else if (eventList[n].data.fd == 0)
                {
                    if(pid != 0)
                    {
                        printf("pushserver> ");
                        fflush(stdout);
                        char btf[MAXBUF];
                        fgets(btf, MAXBUF, stdin);
                        if (!strncasecmp(btf, "quit", 4))
                        {
                            Log("request terminal chat！\n");
                            isQuit = 1;
                            break;
                        }
                    }
                }
                else
                {

                    CLIENT* clientx = (CLIENT*)eventList[n].data.ptr;
                    if(eventList[n].data.ptr < 0) continue;
                    if((sockfd = clientx->fd)<0)
                        continue;
                    int lockret = pthread_mutex_trylock(&clientx->opt_lock);
                    if(lockret == EBUSY)
                    {
                        __DEBUG("client id continue:%d,%s\n",clientx->clientId,clientx->drivceId);
                        continue;
                    }
                    tpool_add_work(read_thread_function,(void*)clientx);
                    //read_thread_function((void*)clientx);
                }
#endif
#ifndef __EPOLL__
                if(FD_ISSET(slisten,&rset)) // new connection
                {
                    int ret = new_connection(slisten,&addr,g_clients,&allset,&maxfd,0);
                    if(ret != 1)
                        continue;
                    //int new_connection(int slisten,struct sockaddr_in* addr,CLIENT* client){
                }
                else if(FD_ISSET(0,&rset))
                {
                    if(pid != 0)
                    {
                        char btf[MAXBUF];
                        fgets(btf, MAXBUF, stdin);
                        if (!strncasecmp(btf, "quit", 4))
                        {
                            Log("request terminal chat！\n");
                            //isQuit = 1;
                            break;
                        }
                    }
                }
                else
                {
                    for(i=0; i<=g_maxi; i++)
                    {
                        if((sockfd = g_clients[i].fd)<0)
                            continue;
                        if(FD_ISSET(sockfd,&rset))
                        {
                            //bzero(buf,MAXBUF + 1);
                            memset(buf,0,MAXBUF+1);
                            if((n = recv(sockfd,buf,MAXBUF,0)) > 0)
                            {
                                Log("received data:%d,from %s:%s\n",n,inet_ntoa(g_clients[i].addr.sin_addr),g_clients[i].drivceId);
                                //void* buffer = buf;
                                CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
                                memset(header,0,sizeof(CLIENT_HEADER));
                                memcpy(header,buf,sizeof(CLIENT_HEADER));

                                void* buffer = malloc(header->totalLength);
                                memcpy(buffer,buf,n);
                                void* tempBuffer = buffer;
                                tempBuffer += n;
                                //more then MAXBUF begin
                                if(header->totalLength>MAXBUF && header->totalLength > n)
                                {
                                    while((n=recv(sockfd,buf,MAXBUF,0))>0)
                                    {
                                        memcpy(tempBuffer,buf,n);
                                        tempBuffer += n;
                                    }
                                }
                                //end

                                buffer = buffer+sizeof(CLIENT_HEADER);
                                char token[255] = {0};
//								char command1[255] = {0};
                                strcpy(token,header->clienttoken);
                                if(header->type == MESSAGE_HELO)
                                {

                                    send_helo(sockfd,header,&g_clients[i],token);

                                }
                                else if(header->type == MESSAGE_PING)
                                {
                                    send_OK(sockfd,MESSAGE_PING);
                                    //save in redis
                                }
                                else if (header->type == MESSAGE)
                                {
                                    int ret = recvMessage(sockfd,redis,header,buffer,&g_clients[i]);
                                    if(ret != 1)
                                    {
                                        free(header);
                                        free(buffer);
                                        break;
                                    }

                                }
                                else if(header->type == MESSAGE_BYE)
                                {
                                    send_OK(sockfd,MESSAGE_BYE);
                                    //remove client from redis
                                    removeClient(&g_clients[i]);
                                    Log("client say goodbye '%s'\n", inet_ntoa(g_clients[i].addr.sin_addr));
//                                    close_socket(sockfd,g_clients,i,&allset,0);
                                    close_socket(&g_clients[i],&allset);
                                }
                                //for cluster
                                /*
                                else if(header->type == MESSAGE_SERVIER){

                                }
                                */
                                else
                                {
                                    //remove client from redis
                                    removeClient(&g_clients[i]);
                                    Log("client say unkown code '%s,len:%d,recv:%s\n", inet_ntoa(g_clients[i].addr.sin_addr),n,buf);
                                    //close_socket(sockfd,g_clients,i,&allset,0);
                                    close_socket(&g_clients[i],&allset);
                                }
                                //Log("received data:%s\n from %s\n",buf,inet_ntoa(client[i].addr.sin_addr));
                                Log("received data:%d,from %s:%s\n",n,inet_ntoa(g_clients[i].addr.sin_addr),g_clients[i].drivceId);

                                free(header);

                                free(buffer);
                            }
                            else
                            {
                                //remove client from redis
                                removeClient(&g_clients[i]);
                                Log("recv data< 0 errno:%d，error msg: '%s'\n", errno, strerror(errno));
                                //close_socket(sockfd,g_clients,i,&allset,0);
                                close_socket(&g_clients[i],&allset);

                            }
                        }
                    }
#endif
                    if(isQuit == 1)
                    {
                        break;
                    }
                }


            }
            if(isQuit == 1)
            {
                break;
            }

        }
        pthread_mutex_init(&st.lock, NULL);
        st.isexit = 1;
        pthread_mutex_unlock(&st.lock);
        pthread_mutex_init(&st2.lock, NULL);
        st2.isexit = 1;
        pthread_mutex_unlock(&st2.lock);
        for(ij=0; ij<(system_config.push_thread_count+system_config.unin_thread_count); ij++)
        {
            pthread_join(coretthreadsId[ij],NULL);
        }

#ifdef __EPOLL__
        close(epollfd);
#endif
        free(coretthreadsId);
        tpool_destroy();
        destroy_redis_pool();
        close(slisten);
        char pragme[500];
        getRealPath(pragme);
        strcat(pragme,"/");
        strcat(pragme,"pushserver.lock");
        deletePidFile(pragme);
#ifdef CHECKMEM
        atexit(report_mem_leak);
#endif
    }


    void* sendMessage(void* args)
    {
        send_info_t* sendinfo = (send_info_t*)args;
        if(sendinfo == NULL)
            return NULL;
        pthread_mutex_lock(&sendinfo->client->opt_lock);
        int ret = send_pushlist_message(sendinfo->info,sendinfo->client);
        if(sendinfo->info){
			freePushMessage(sendinfo->info);
        }
        if(ret<=0)
        {
            saveInNoread(sendinfo->client->drivceId,sendinfo->info->messageid);
        }
        pthread_mutex_unlock(&sendinfo->client->opt_lock);
        return NULL;
    }
    void* send_helo_to_client_message(CLIENT* client)
    {
        char* token = client->drivceId;
        char pushid[255]= {0};
        int ret = 0;
        do
        {
            ret = getNextNoReadMessageId(client->drivceId,pushid);
            if(ret <= 0)
                break;
            push_message_info_t* messageinfo = NULL;
            getmessageinfo(pushid,&messageinfo);
            if(messageinfo == NULL)
//				goto next_while;
                continue;

            int lockret = pthread_mutex_lock(&client->opt_lock);
            //if(lockret == EBUSY)
            //	continue;
            int isHasClient = send_pushlist_message(messageinfo,client);
            freePushMessage(messageinfo);
            pthread_mutex_unlock(&client->opt_lock);
            //Log("isHasClient=%d,errno:%d,error msg: '%s'\n",isHasClient, errno, strerror(errno));
            if(isHasClient > 0)
            {

            }
        }
        while(ret>0);
    }

    void send_helo(int sockfd,CLIENT_HEADER	*header,CLIENT* client,char* deviceid)
    {
        //save in redis header infomation
        redisReply* reply = NULL;
        char time[255];
        formattime(time,NULL);
        struct sockaddr_in addr;
        socklen_t slen;
        getsockname(sockfd,(struct sockaddr*)&addr,&slen);
        strcpy(client->drivceId,deviceid);
        client_info_t* clientinfo = NULL;
        newClient(client,system_config.serverid,inet_ntoa(addr.sin_addr),
                  client->drivceId,
                  1, /* TODO is need get from client header from client */
                  &clientinfo);
        freeClientInfo(clientinfo);
        /////////////////////////////////////////////////////////////////
        send_OK(client->fd,MESSAGE_HELO);

        send_helo_to_client_message(client);
    }

    void close_socket(CLIENT* client,fd_set* allset)
    {
        if(active_socket_connect<1)
            return;
        //CLIENT* clientx = &client[clientId];
        Log("%s:%d disconnected by client!\n",inet_ntoa(client->addr.sin_addr),client->addr.sin_port);
        int sockfd = client->fd;
        int clientid = client->clientId;
        //TODO需要重新排序
        if(active_socket_connect>=2)
        {
            pthread_mutex_lock(&client_lock);
            
#ifdef __EPOLL__
            /*
                	    struct epoll_event event;
                		event.data.ptr = clientx;
                		event.events =  EPOLLIN|EPOLLET;  //EPOLLET
                		epoll_ctl(epollfd, EPOLL_CTL_MOD, clientx->fd, &event);
            */
            epoll_ctl(g_epollfd,  EPOLL_CTL_DEL, client->fd, NULL);
            /*
            //epoll_ctl(g_epollfd,  EPOLL_CTL_DEL, g_clients[active_socket_connect-1].fd, NULL);
            //memcpy(client,&g_clients[active_socket_connect-1],sizeof(CLIENT));
            //client->clientId = clientid;
            //changeClientId(client->drivceId,clientid);
            //addepollevent(client->fd,client);
            //client[active_socket_connect-1].fd = -1;
            //g_maxi--;
            //maxi--;
            */
            client->fd = -1;
            memset(client->drivceId,0,sizeof(client->drivceId));
            active_socket_connect--;
            
#endif
            pthread_mutex_unlock(&client_lock);
        }
        else if(active_socket_connect == 1)
        {
            pthread_mutex_lock(&client_lock);
            active_socket_connect--;
            //g_maxi--;

#ifdef __EPOLL__
            epoll_ctl(g_epollfd,  EPOLL_CTL_DEL, client->fd, NULL);
#endif
            client->fd = -1;
            pthread_mutex_unlock(&client_lock);
        }

        close(sockfd);
        FD_CLR(sockfd,allset);


    }
    /*****************************
     *
     *list pushlist
      {
      	messageid(guid)
      }
     *
     *list device_noread ( recv device list)
      {
      	messageid(guid)
      }
     *
     *hash messageid(guid)
      {
      	from:from device
      	to:  recv device
      	fromip:from client ip
      	state:0(no read),1(read)
      	content:messages
      	sendtime:send time
      	retrycount:0(try to send count)
      	messagetype:0xfxx
      	EXPIRE:120(TODO)
      }
     *
     *
     ***********************/
    int recvMessage(int sockfd,redisContext* redis,CLIENT_HEADER *header,void* buffer_header,CLIENT* client)
    {
        char guid[32]= {0};
        redisReply* reply = NULL;
        int sendto = header->sendto;
        //int userid = header->userid;
        char tochar[64] = {0};
        char fileName[255]= {0};
        char newFilename[500]= {0};
        int fileNameLength = 255;
        void* message = NULL;
        void* buffer = buffer_header;
        if(header->messagetype != MESSAGE_TYPE_TXT)
        {
            memcpy(fileName,buffer,fileNameLength);
        }
        message =  malloc(header->contentLength);
        if(message == 0)
        {
            Log("the message is malloc error\n");
            return 0;
        }
        memset(message,0,header->contentLength);
        if(header->messagetype == MESSAGE_TYPE_TXT)
        {
            memcpy(message,buffer+sendto*64,header->contentLength);
            char* tmpMessage = (char*)message;
            tmpMessage[header->contentLength] = '\0';
//            message[header->contentLength] = '\0';
        }
        else
        {
            memcpy(message,buffer+sendto*64+fileNameLength,header->contentLength);
            char tempStr[500]= {0};
            sprintf(tempStr,"%s",fileName);
            sprintf(tempStr,"%s%s",tempStr,time);
            createMd5(tempStr,newFilename);
            //TODO save file and create new filename put in messageinfo
            //TODO in future create another socket listen or http service to store file,the content will may new file path
            // http://xxxxxxx,or /tmp/xxxxxx
            writetofile(system_config.tempPath,newFilename,message,header->contentLength);
        }
        if(sendto > 0)
        {
            int j=0;
            for(j=0; j<sendto; j++)
            {
                memset(tochar,0,64);
                if(header->messagetype == MESSAGE_TYPE_TXT)
                    memcpy(tochar,buffer+j*64,64);
                else
                    memcpy(tochar,buffer+j*64+fileNameLength,64);
                push_message_info_t* info = NULL;
                putmessageinfo(//guid,
                    message,tochar,fileName,newFilename,300,//
                    header,client,0,&info);
                print_push_info(info);
                freePushMessage(info);

            }
            Log("From %s ,Rect message:%s \n",inet_ntoa(client->addr.sin_addr),message);
        }
        else if(sendto == 0)
        {
            //globle message?
        }
        else
        {
            //error
        }

        //Log("received data:%s\n from %s \n",message,inet_ntoa(client->addr.sin_addr));
//        Log("received data:--from %s:%s\n",inet_ntoa(client->addr.sin_addr),client->drivceId);
        if(message != 0)
        {
            free(message);
            //message = 0;
        }
        /*
        	struct epoll_event event;
        	event.data.ptr = client;
        	event.events =  EPOLLIN|EPOLLET;  //EPOLLET
        	epoll_ctl(epollfd, EPOLL_CTL_MOD, client->fd, &event);
        */
        send_OK(sockfd,MESSAGE_YES);
        return 1;
    }
    void send_OK(int sockfd,int type)
    {
        SERVER_HEADER* sheader = malloc(sizeof(SERVER_HEADER));
        sheader->messageCode = MESSAGE_SUCCESS;
        if(type != -1)
            sheader->type = type;
        else
            sheader->type = S_OK;
        sheader->messagetype = MESSAGE_TYPE_TXT;
        sheader->totalLength = sizeof(SERVER_HEADER);
        int len = send(sockfd, sheader, sizeof(SERVER_HEADER) , 0);
        //Log("len=%d:%d ,errno:%d，error msg: '%s'\n",len,sizeof(SERVER_HEADER) , errno, strerror(errno));

        free(sheader);
    }
    int new_connection(int slisten,struct sockaddr_in* addr,CLIENT* client,fd_set* allset,int* maxfd,int epollfd)
    {
        socklen_t len;
        //fd_set rset,allset;
        len = sizeof(struct sockaddr);
        int connectfd;
        if((connectfd = accept(slisten,
                               (struct sockaddr*)addr,&len)) == -1)
        {
            perror("accept() error\n");
            return 0;
        }
        struct timeval timeout =
        {
            1,0
        };
        setsockopt(connectfd,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(struct timeval));
//    	setsockopt(connectfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(struct timeval));
        int i = 0;
        pthread_mutex_lock(&client_lock);
#ifdef __EPOLL__
        for(i=0; i<MAX_CONNECTION_LENGTH; i++)
#else
        for(i=0; i<FD_SETSIZE; i++)
#endif
        {
            if(client[i].fd < 0)
            {
                client[i].fd = connectfd;
                //client[i].clientid=autoclientid;
                client[i].addr = *addr;
                pthread_mutex_init(&client[i].opt_lock , NULL);
                Log("Yout got a connection from %s.\n",
                    inet_ntoa(client[i].addr.sin_addr));
                active_socket_connect++;
                Log("now connection user is %d\n",active_socket_connect);
                //autoclientid++;
                break;
            }
        }
        pthread_mutex_unlock(&client_lock);
#ifdef __EPOLL__
        if(i == MAX_CONNECTION_LENGTH)
#else
        if(i == FD_SETSIZE)
#endif
            Log("too many connections");
        pthread_mutex_lock(&client_lock);
#ifdef __EPOLL__
        addepollevent(connectfd,&client[i]);
#else
        FD_SET(connectfd,allset);
#endif
        if(connectfd > *maxfd)
            *maxfd = connectfd;
        if(i > g_maxi)
            g_maxi = i;
        pthread_mutex_unlock(&client_lock);
        return 1;

    }


    void* read_thread_function(void* client_t)
    {
        void* buf = malloc(MAXBUF);
        int sockfd;
        CLIENT* clientx = (CLIENT*)client_t;
        sockfd = clientx->fd;
        int n  = -1;
        memset(buf,0,MAXBUF + 1);
        int redisID;
        redisContext* redis = getRedis(&redisID);
        int reads = 1;
        int epollfd = g_epollfd;
        if((n = recv(sockfd,buf,MAXBUF,0)) > 0)
        {
            //TODO Judging communication protocol, custom,websocket,xmpp or SSL
            //if (strlen(buf) < 14) {
            //  it is may SSL
            //	Log("SSL request is not supported yet.\n");
            //}
            if(strncasecmp(buf,"get",3) == 0)
            {
                //TODO it is may websocket

            }
            else if(strncasecmp(buf,"<",1) == 0)
            {
                //TODO it is may xmpp
            }
            //here is custom protocol in read_thread_function
            //dump_data(buf,n);
            CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
            memset(header,0,sizeof(CLIENT_HEADER));
            memcpy(header,buf,sizeof(CLIENT_HEADER));
            void* buffer = NULL;
            void* tempBuffer = NULL;
            if(header->totalLength>0)
            {
                buffer = malloc(header->totalLength);
                memcpy(buffer,buf,n);
                tempBuffer = buffer;
            }
            //Log("received data from  %s:%s type is:%02X,mtype is:%02X\n",inet_ntoa(clientx->addr.sin_addr),clientx->drivceId,header->type,header->messagetype);
            //more then MAXBUF begin
            int sum = n;
            //Log("header->totalLength:%d,n=%d\n",header->totalLength,n);
            if(header->totalLength>MAXBUF && header->totalLength > n)
            {
                while(1)
                {
                    int tempret = 0;
                    memset(buf,0,MAXBUF);
                    if((header->totalLength-sum)>MAXBUF)
                    {
                        tempret = recv(sockfd,buf,MAXBUF,0);
                        if(tempret <=0)
                        {
                            goto recv_error_close_client;
                        }

                        Log("recv length=%d\n",tempret);
                    }
                    else
                    {
                        tempret = recv(sockfd,buf,header->totalLength-sum,0);
                        Log("recv length=%d\n",tempret);
                        if(tempret <=0)
                        {
                            goto recv_error_close_client;
                        }
                        memcpy(tempBuffer+sum,buf,tempret);
                        sum+=tempret;
                        if(header->totalLength-sum == 0)
                            break;
                        else
                            continue;
                    }
                    if(tempret < 0)
                    {
                        goto recv_error_close_client;
                    }
                    if(tempret >0)
                        memcpy(tempBuffer+sum,buf,tempret);
                    sum += tempret;
                }
            }
            pthread_mutex_unlock(&clientx->opt_lock);
            //Log("totalLength=%d,sum=%d\n",header->totalLength,sum);
            //end
            char token[255] = {0};
            strcpy(token,header->clienttoken);
            if(header->type == MESSAGE_HELO)
            {
                send_helo(sockfd,header,clientx,token);
            }
            else if(header->type == MESSAGE_PING)
            {
                send_OK(sockfd,MESSAGE_PING);
                //save in redis
            }
            else if (header->type == MESSAGE)
            {
                void* recvBuf = buffer;
                if(buffer != NULL)
                    recvBuf = recvBuf+sizeof(CLIENT_HEADER);
                int ret = recvMessage(sockfd,redis,header,recvBuf,clientx);
                if(ret != 1)
                {
                    //if(buffer != NULL)
                    //	free(buffer);
                    //break;
                }
            }
            else if(header->type == S_OK)
            {

            }
            else if(header->type == MESSAGE_BYE)
            {
                send_OK(sockfd,MESSAGE_BYE);
                Log("client say goodbye '%s'\n", inet_ntoa(clientx->addr.sin_addr));
                goto recv_error_close_client;
            }
            /*
            else if(header->type == SERVIER){

            }
            */
            else
            {
                Log("client say unkown code:%d ,%s,len:%d,recv:%s\n",header->type,inet_ntoa(clientx->addr.sin_addr),n,buf);
recv_error_close_client:
                pthread_mutex_unlock(&clientx->opt_lock);
                removeClient(clientx);
                //close_socket(sockfd,g_clients,clientx->clientId,&allset,epollfd);
                close_socket(clientx,&allset);

            }
            if(header->totalLength>0 && buffer != NULL)
            {
                free(buffer);
            }
            free(header);
        }
        else
        {
            pthread_mutex_unlock(&clientx->opt_lock);
            Log("recv data< 0 errno:%d，error msg: '%s'\n", errno, strerror(errno));
            removeClient(clientx);
            //close_socket(sockfd,g_clients,clientx->clientId,&allset,epollfd);
            close_socket(clientx,&allset);
        }

        free(buf);
        returnRedis(redisID);
        return NULL;
    }

    void* pushlist_send_thread(void* stx)
    {
        pushstruct* st = (pushstruct*)stx;
        Log("Connect to redisServer Success\n");
        while(st->isexit == 0)
        {
            char messageid[255] = {0} ;
            int ret = 0;
            do
            {
                int ret = getNextMessageId(system_config.serverid,messageid);
                if(ret <= 0)
                    break;
                int mm=0;
                int isHasClient = 0;

                push_message_info_t* messageinfo = NULL;
                getmessageinfo(messageid,&messageinfo);
                if(messageinfo == NULL)
                {
                    //goto pop_pushlist;
                    continue;
                }
                client_info_t* clientinfo = NULL;
                getClientInfo(&clientinfo,messageinfo->to);
                //print_client_info(clientinfo);
                if(clientinfo == NULL)
                {
                    freePushMessage(messageinfo);
                    goto save_dely_push;
                }
                if(messageinfo == NULL)
                {
save_dely_push:
                    saveInNoread(messageinfo->to,messageid);
pop_pushlist:
                    continue;
                }
                
                /*
                CLIENT clientx;
                pthread_mutex_lock(&client_lock);
				memcpy(&clientx,&st->client[clientinfo->clientid],sizeof(CLIENT));
				if(strncasecmp(clientx.drivceId,clientinfo->drivceId,strlen(clientx.drivceId)!=0))
					goto save_dely_push;
				pthread_mutex_unlock(&client_lock);
				*/
				CLIENT* clientx = &st->client[clientinfo->clientid];
                if(clientx->fd != -1)
                {
                    //TODO need to create a thread to send message
                    //isHasClient = send_pushlist_message(messageinfo,clientx);
                    send_info_t sendinfo;
                    sendinfo.info = messageinfo;
                    sendinfo.client = clientx;
                    tpool_add_work(sendMessage,&sendinfo);
                }else{
                	goto save_dely_push;
                }
                //delete to Improve the system performance
                /*

                #ifndef __EPOLL__
                for(mm=0; mm<FD_SETSIZE; mm++)
                #endif
                #ifdef __EPOLL__
                    for(mm=0; mm<MAX_EVENTS; mm++)
                #endif
                    {
                		clientx = &st->client[mm];
                        if(clientx->fd<0) break;
                        char* cip = NULL;
                        int cport = -1;
                        cport = clientx->addr.sin_port;
                        cip = inet_ntoa(sclientx->addr.sin_addr);
                        if(strcmp(cip,clientinfo->clientip) == 0 && cport == clientinfo->clinetport)
                        {
                            //send message to client
                            printf(">>--------------------start push message--------------------------<<\n");
                            print_push_info(messageinfo);
                            printf(">>------------------------end-------------------------------------<<\n");
                            isHasClient = send_pushlist_message(messageinfo,clientinfo,clientx);
                            Log("isHasClient=%d,errno:%d,error msg: '%s'\n",isHasClient, errno, strerror(errno));
                            break;
                        }
                    }
                    if(isHasClient == 0)
                	{
                   	 	goto save_dely_push;
                	}
                */
                //here will delete in thread
                //freePushMessage(messageinfo);
                freeClientInfo(clientinfo);


                if(st->isexit != 0)
                {
                    Log("------------pushlist_send_thread thread exit------------------\n");
                    goto exit_send_funtion;
                }
            }
            while(ret>0);
            int ij = 0;
            for(ij=0; ij<st->timeout; ij++)
            {
                if(st->isexit != 0)
                {
                    Log("------------pushlist_send_thread thread exit------------------\n");
                    goto exit_send_funtion;
                }
                sleep(1);
            }

        }
exit_send_funtion:
        Log("------------pushlist_send_thread thread exit------------------\n");
    }

    int send_pushlist_message(
        push_message_info_t* messageinfo,
        CLIENT* client)
    {
        int ret = 0;
        int sendlen = sizeof(SERVER_HEADER);//+content!=NULL?strlen(content):0;
        SERVER_HEADER* sheader = malloc(sizeof(SERVER_HEADER));
        sheader->messageCode = MESSAGE_SUCCESS;
        sheader->type = MESSAGE;
        sheader->messagetype = messageinfo->messagetype;

        if(messageinfo->content!=NULL && messageinfo->messagetype == MESSAGE_TYPE_TXT)
        {
            sendlen += strlen(messageinfo->content);
            sheader->contentLength = strlen(messageinfo->content);
        }
        else if( messageinfo->content!=NULL && messageinfo->messagetype != MESSAGE_TYPE_TXT)
        {
            long filesize = get_file_size(messageinfo->content);
            sendlen += 255;
            sendlen += filesize;
            sheader->contentLength = (int)filesize;
        }
        else if(messageinfo->content == NULL)
            sheader->contentLength = 0;
        sheader->totalLength = sendlen;
        //TODO
        void* sendbuffer = malloc(sendlen);
        memset(sendbuffer,0,sendlen);
        memcpy(sendbuffer,sheader,sizeof(SERVER_HEADER));
        if(messageinfo->content != NULL)
        {
            if(messageinfo->messagetype != MESSAGE_TYPE_TXT)
            {
                memcpy(sendbuffer+sizeof(SERVER_HEADER),messageinfo->orgFileName,255);
                int ret;
                unsigned char* filecontent = readtofile(system_config.tempPath,messageinfo->content,&ret);
                memcpy(sendbuffer+sizeof(SERVER_HEADER)+255,filecontent,ret);

                free(filecontent);
            }
            else
            {
                memcpy(sendbuffer+sizeof(SERVER_HEADER),messageinfo->content,strlen(messageinfo->content));
            }

        }
        int len = -1;
        if(sendlen>MAXBUF)
        {
            int sumlen = 0;
            int i = 0;
            while(sumlen < sendlen)
            {
                int retsend = 0;
                if((sendlen-sumlen)>MAXBUF)
                    retsend = send(client->fd, sendbuffer+sumlen,MAXBUF,0);
                else
                    retsend = send(client->fd, sendbuffer+sumlen,(sendlen-sumlen),0);
                if(retsend<0)
                {
                    goto send_error;
                }
                sumlen += retsend;
                Log("send data length=%d\n",sumlen);
                i++;
            }
            len = sumlen;
        }
        else
        {
            len = send(client->fd, sendbuffer, sendlen, 0);
        }
        Log("Send message:%s to %s len:%d,errno:%d,error msg: '%s'\n",messageinfo->content,client->drivceId,len, errno, strerror(errno));
        free(sendbuffer);
        sendbuffer = NULL;
        free(sheader);
        sheader = NULL;
        //TODO
        if(len < 0)
        {
send_error:
            removeClient(client);
            Log("send error and say goodbye '%s'\n", inet_ntoa(client->addr.sin_addr));
            //close_socket(client->fd,g_clients,client->clientId,&allset,g_epollfd);
            close_socket(client,&allset);
            return 0;
        }
        //wait for client return
        char tempbuf[MAXBUF];
        len = recv(client->fd, tempbuf, MAXBUF, 0);
        if(len>0)
        {
            CLIENT_HEADER* cheader = malloc(sizeof(CLIENT_HEADER));
            memset(cheader,0,sizeof(CLIENT_HEADER));
            memcpy(cheader,tempbuf,sizeof(CLIENT_HEADER));
            if(cheader->type == S_OK)
            {
                ret = 1;
                messageIsSendOK2( messageinfo,system_config.serverid);
            }

            free(cheader);
        }
        else
        {
            ret = 0;
            return ret;
        }
        return ret;
    }
//
    void* unionPushList(void* stx)
    {
        struct pushstruct* stn;
        stn = (struct pushstruct*)stx;
        Log("Connect to redisServer Success\n");
        while(1)
        {
            char messageid[255] = {0};
            char drivceid[255] = {0};
            int ret = 0;
            do
            {
                ret = getDelyMessage(messageid);//TODO this method has bug!
                if(ret > 0 )
                {
                    int ret2 = getClientDrivceIdFromMessage(messageid,drivceid);
                    if(ret2 <= 0)
                        continue;
                    client_info_t* clientinfo = NULL;
                    //ret2 = isClientOnline(drivceid)
                    getClientInfo(&clientinfo,drivceid);
                    if(clientinfo != NULL)
                    {
                        //for cluster
                        saveInPushlist(messageid,clientinfo->serverid);
                    }
                    else
                    {
                        saveInNoread(drivceid,messageid);
                    }
                    freeClientInfo(clientinfo);
                }
            }
            while(ret > 0);
            /*
            redisReply* reply = (redisReply*)redisCommand(redis, "RPOPLPUSH dely_pushlist pushlist");
            while(reply->str != NULL)
            {
                _freeReplyObject(reply);
                if(stn->isexit != 0)
                {
                    Log("------------union thread exit------------------\n");
                    returnRedis(redisID);
                    return NULL;
                }
                reply = (redisReply*)redisCommand(redis, "RPOPLPUSH dely_pushlist pushlist");
            }
            _freeReplyObject(reply);
            */
            int ij = 0;
            for(ij=0; ij<stn->timeout; ij++)
            {
                if(stn->isexit != 0)
                {
                    Log("------------union thread exit------------------\n");
                    return NULL;
                }
                sleep(1);
            }
        }
    }
    /*
    void createsig(int sig){

    	struct sigaction act;

        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = new_op;
        sigaction((int)p, &act, NULL);
        //signal(SIGALRM, ouch);
        printf("\nsigal = %d\n", (int)p);

        signal(SIGHUP, ouch);
    } */

    //create md5
    /*
    char *s = "123456";
    unsigned char ss[16];
    MD5Init( &md5c );
    char bufx[33]={'\0'};
    char tmp[3]={'\0'};
    MD5Update( &md5c, s, strlen(s) );
    MD5Final( ss, &md5c );
    printf("%s\n",ss);
    for( i=0; i<16; i++ ){
    sprintf(tmp,"%02X", ss[i] );
    strcat(bufx,tmp);
    }
    printf("%s",bufx);
    printf( "\t%s\n", s );
    */