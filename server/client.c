#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "log.h"
#include "sc.h"
#include "parse_command.h"
#include "list.h"
#include "extended_protocol.h"

//#define MAXBUF 1024
#define WIN32


#ifdef Log
#undef Log
#endif
#ifndef Log
#define Log(format,...) NULL
#endif

#ifndef DEBUG
#define DUMP_DT(buf,len) NULL
#else
#define DUMP_DT(buf,len) dump_data(buf,len)
#endif
#define __DEBUG(format, ...) printf("FILE: "__FILE__", LINE: %d: "format"/n", __LINE__, ##__VA_ARGS__)

typedef struct pushstruct
{
    CLIENT* client;
    pthread_mutex_t lock;
    int isexit;
    int timeout;
} pushstruct;
typedef struct thread_struct
{
    int sockfd;
    pthread_mutex_t lock;
    int isexit;
    char token[255];
} thread_struct;

pthread_mutex_t lock;
int g_sock;
void recvReturn(int sockfd)
{
    char buf[MAXBUF];
    int ret = recv(sockfd,buf,MAXBUF,0);
    if(ret > 0)
    {
        printf("recv return message:\n");
        dump_data(buf,ret);
    }
    else{
    	printf("recv error:%d",ret);
    	
    }
}
void sendhelo(int sockfd,char* token)
{
	pthread_mutex_lock(&lock);
    void* buff = NULL;
    int length = createClientHelo(&buff,1,token,NULL);
    printf("helo\n");
    dump_data(buff,length);
    int len = send(sockfd, buff, length, 0);
    free(buff);
    if (len > 0)
    {
        Log("msg:%s send successful，totalbytes: %d！\n", buff, len);
    }
    else
    {
        printf("msg:'%s  failed!\n", buff);
    }
    recvReturn(sockfd);
    pthread_mutex_unlock(&lock);

}

void sendping(int sockfd,char* token)
{
	
	pthread_mutex_lock(&lock);
    void* buff = createClientPing(1);
    int len = send(sockfd, buff, sizeof(client_header_2_t), 0);
    printf("ping\n");
    dump_data(buff,len);
    free(buff);
    if (len > 0)
        Log("msg:%s send successful，totalbytes: %d！\n", buff, len);
    else
    {
        printf("msg:'%s  failed!\n", buff);
    }
    recvReturn(sockfd);
    pthread_mutex_unlock(&lock);

}
/**
 * time to ping server
 */
void thread(thread_struct* thread_s)
{
    while(thread_s->isexit == 0)
    {

        int i = 0;
        for(i = 0; i<100; i++)
        {
            if(thread_s->isexit == 0)
                sleep(1);
            else
                return;
        }
        sendping(thread_s->sockfd,thread_s->token);

    }
}

void* client_thread(char **argv)
{

    //char totoken_temp[64]={0};
    int isQuit = 0;
    while(isQuit == 0)
    {
        int sockfd, len;
        struct sockaddr_in dest;
        char bufs[MAXBUF + 1];
        char token[MAXBUF];
        fd_set rfds;
        struct timeval tv;
        int retval, maxfd = -1;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("Socket");
            exit(errno);
        }

        bzero(&dest, sizeof(dest));
        bzero(&token,sizeof(token));
        strcpy(token,argv[3]);
        char* tempArgv = malloc(strlen(argv[4]));
        strcpy(tempArgv,argv[4]);
        int totokenLength = splitcountx(argv[4],"_");
        char** totoken_temp = NULL;
        totoken_temp = malloc(totokenLength);
        int i =0;
        for(i=0; i<totokenLength; i++)
        {
            totoken_temp[i] = malloc(64);
        }
        totokenLength = split(totoken_temp,tempArgv,"_");
        free(tempArgv);
        //bzero(&totoken_temp,sizeof(totoken_temp));

        //strcpy(totoken_temp,argv[4]);
        dest.sin_family = AF_INET;
        dest.sin_port = htons(atoi(argv[2]));
        if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0)
        {
            perror(argv[1]);
            exit(errno);
        }

        if(connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0)
        {
            perror("Connect ");
            exit(errno);
        }
        g_sock = sockfd;
        printf("connect to server port:%d...\n",dest.sin_port);

        sendhelo(sockfd,token);
        //clock_t start = clock();
        //start ping thread
        thread_struct st;
        st.sockfd = sockfd;
        st.isexit = 0;
        strcpy(st.token,token);
        pthread_t thread_id;
        pthread_mutex_init(&st.lock, NULL);
        int ret=pthread_create(&thread_id,NULL,(void *) thread,&st);
        pthread_mutex_unlock(&st.lock);
        while (1)
        {

            FD_ZERO(&rfds);
            //FD_SET(0, &rfds);
            maxfd = 0;

            FD_SET(sockfd, &rfds);
            if (sockfd > maxfd)
                maxfd = sockfd;

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);

            if (retval == -1)
            {
                printf("select error! %s", strerror(errno));
                break;
            }
            else if (retval == 0)
            {
                //printf("no msg,no key, and continue to wait…??\n");
                continue;
            }
            else
            {
                if (FD_ISSET(0, &rfds))
                {

                }
                else if (FD_ISSET(sockfd, &rfds))
                {
                	
                    bzero(bufs, MAXBUF + 1);
                    int lockret = pthread_mutex_trylock(&lock);
                    if(lockret == EBUSY)
                    	continue;
                    len = recv(sockfd, bufs, MAXBUF, 0);                    
                    //printf("recv data recv len:%d,SERVER_HEADER len:%d\n",len,sizeof(SERVER_HEADER));
                    printf("recv:\n");
                    char* buf = bufs;
                    dump_data(buf,len);
                    //SERVER_HEADER* header = malloc(sizeof(SERVER_HEADER));
                    //bzero(header,sizeof(SERVER_HEADER));
                    server_header_2_t header;
                    memset(&header,0,sizeof(server_header_2_t));
                    if (len > 0)
                    {
                        void* buff = parseServerHeader(buf,&header);
                        printf("the command %02X\n",header.command);
                        if(len>=sizeof(server_header_2_t))
                        {
                            //memcpy(&header,buf,sizeof(server_header_2_t));
                            buf += sizeof(server_header_2_t);
                            len -= sizeof(server_header_2_t);
                            if(header.command == COMMAND_MESSAGE)
                            {
                                char fromdrivceId[64]= {0};
                                char messageid[64]= {0};
                                char* message = NULL;
                                int ret = parseServerMessage(sockfd,buff,
                                                             &header,fromdrivceId,messageid,&message,len,"/tmp");
                                if(ret < 0)
                                {
                                    printf("has error\n");
                                }
                                printf("%s,%s,%s",messageid,fromdrivceId,message);
                                free(message);
                                char* retbuf = createClientHeader(COMMAND_YES,MESSAGE_TYPE_NULL,(unsigned char)1);
                                ret = send(sockfd,retbuf,sizeof(client_header_2_t),0);
                                if(ret < 0)
                                {
                                    printf("has error\n");
                                }
                            }
                        }
                        else
                        {
							Log("total:%s %d \n",__LINE__,len);
                        }
                        //Log("recv:'%s, total: %d,%d \n", buf, len,header->messageCode);
                    }
                    else
                    {
                        if (len < 0)
                            printf("recv failed！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                        else
                        {
                            printf("recv failed！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                            printf("other exit，terminal chat\n");
                        }


                        //  free(header);
                        break;
                        //continue;
                    }
                    // free(header);
                    
                    pthread_mutex_unlock(&lock);
                }
            }
        }

        close(sockfd);
        pthread_mutex_init(&st.lock, NULL);
        st.isexit = 1;
        pthread_mutex_unlock(&st.lock);
        pthread_join(thread_id,0);
    }
    return 0;
}
/*
int main(int argc,char** argv){
	push_message_info_t info;
	info.retrycount=0;
	info.state=0;
	info.messagetype=MESSAGE_TYPE_TXT;
	strcpy(info.messageid , "hahah");
	strcpy(info.to,"kuer");
	strcpy(info.orgFileName , "");
	strcpy(info.sendtime,"2015");
	info.sendtime_l = 131;
	strcpy(info.form,"test");
	info.content = malloc(20);
	strcpy(info.content,"111");
	info.timeout = 100;	
	int i =0;
	for(i=0;i<10;i++)
		createServerMessagereply(-1,1,&info,"");
}
*/
int main(int argc, char **argv)
{

    if (argc <= 3)
    {
        printf("Usage: %s IP Port token",argv[0]);
        exit(0);
    }
    char token[64];
    bzero(&token,strlen(token));
    strcpy(token,argv[3]);
    char* tempArgv = malloc(strlen(argv[4]));
    strcpy(tempArgv,argv[4]);
    int totokenLength = splitcountx(argv[4],"_");
    char** totoken_temp = NULL;
    totoken_temp = malloc(totokenLength);
    int i =0;
    for(i=0; i<totokenLength; i++)
    {
        totoken_temp[i] = malloc(64);
    }
    totokenLength = split(totoken_temp,tempArgv,"_");
    free(tempArgv);
	pthread_mutex_init(&lock,NULL);
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, (void *) client_thread, argv) != 0)
    {
        printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
               errno, strerror(errno));
        exit(1);
    }
    //int ret=pthread_create(&thread_id,NULL,(void *) thread,&st);
    char buf[MAXBUF];
    char username[255]={0};
    while(1)
    {
        bzero(buf, MAXBUF + 1);
        fgets(buf, MAXBUF, stdin);
        char filepath[500]= {0};
        int isfile = 0;
        if (!strncasecmp(buf, "quit", 4))
        {

            printf("request terminal chat！\n");
            exit(0);
        }
        if (!strncasecmp(buf, "exit", 4))
        {
            printf("request terminal chat by exit！\n");
            break;
        }
        if(!strncasecmp(buf,"file",4))
        {
            //TODO 获得文件
            sscanf(buf,"%*s%s",filepath);
            printf("%s\n",filepath);
            isfile = 1;
        }
        if(!strncasecmp(buf,"reg",3)){
        	
        	sscanf(buf,"%*s%s",username);
        	printf("username");
			pthread_mutex_lock(&lock);
			createClientUserReg(g_sock,1,username);
			__DEBUG("\n");
			recvReturn(g_sock);
			pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"login",5)){
			pthread_mutex_lock(&lock);
			createClientUserLogin(g_sock,1,username);
			recvReturn(g_sock);
			pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"find",4)){
        	char fuser[255];
        	sscanf(buf,"%*s%s",fuser);
			pthread_mutex_lock(&lock);
			createClientUserFindUser(g_sock,1,fuser);
			recvReturn(g_sock);
			pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"add",3)){
        	char fuser[255];
        	sscanf(buf,"%*s%s",fuser);
			pthread_mutex_lock(&lock);
			//createClientUserFindUser(g_sock,1,fuser);
			recvReturn(g_sock);
			pthread_mutex_unlock(&lock);
        	continue;
        }
        int len = -1;
        list_t *send_list = list_new();
        int j=0;
        for(j=0; j<totokenLength; j++)
        {
            list_node_t *node = list_node_new(totoken_temp[j]);
            list_rpush(send_list, node);
        }
        int trylock = pthread_mutex_trylock(&lock);
        if(trylock == EBUSY){
        	pthread_mutex_lock(&lock);
        }			
        if(isfile == 0)
            len = createClientMessage(g_sock,MESSAGE_TYPE_TXT,1,0,buf,1,send_list);
        else
            len = createClientMessage(g_sock,MESSAGE_TYPE_FILE,1,0,filepath,1,send_list);
        recvReturn(g_sock);
		pthread_mutex_unlock(&lock);
        list_destroy(send_list);
        if (len > 0)
        {
            Log("msg:%s send successful，totalbytes: %d,%d！\n", buf, len,length);
        }
        else
        {
            printf("msg:'%s  failed!\n", buf);
            break;
        }

    }
    pthread_join(thr_id,NULL);
}

