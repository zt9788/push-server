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
#include "client.h"

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

#define CLIENT_TYPE 1



pthread_mutex_t lock;
pthread_mutex_t send_lock;
pthread_cond_t  queue_send;

int g_sock;
int g_isexit = 0;
pthread_t core_thread_id;
pthread_t ping_thread_id;
pthread_t send_thread_id;
list_t* sendlist;



void* send_message_processer(void* data)
{
//	sendmessage_stuct_t* sst = args;
	//JNIEnv* env = (JNIEnv*)sst->env;
    send_message_t* head = NULL;//sendlist;
    while(g_isexit == 0)
    {
    	//LOGI("qidongle");
        pthread_mutex_lock(&send_lock);        
        pthread_cond_wait(&queue_send, &send_lock);
            //LOGI("jinrule");
         while(sendlist->len>0)
        {
			list_node_t* node = list_rpop(sendlist);
			send_message_t* sendmessage = node->val;
            int len = 0;
          
            //LOGI("total filesize %d",length);
            pthread_mutex_lock(&lock);
            createClientMessage(g_sock,
								CLIENT_TYPE,sendmessage->messagetype,
								sendmessage->delytime,sendmessage->message,sendmessage->sendto);

            //recv response
            unsigned char rebuf[MAXBUF];
            len = recv(g_sock,rebuf,MAXBUF,0);
            if(len<0){
            	goto send_error2;
            }
            server_header_2_t header;
            memset(&header,0,sizeof(server_header_2_t));
            void* buffrecv = parseServerHeader(rebuf,&header);
            char messageid[64]= {0};
			parseServerMessageReply(buffrecv,messageid);
			
send_error2:
			
			pthread_mutex_unlock(&lock);
            pthread_mutex_unlock(&send_lock);
            //call java function to show the message is send ok
            if(len<0)
				sendMessageResponse(sendmessage->messageid,messageid,0,"send error",data);
			else
				sendMessageResponse(sendmessage->messageid,NULL,1,"",data);
				
            free(sendmessage->message);
            list_destroy(sendmessage->sendto);
            free(sendmessage);
        }
    }
    return NULL;
}

void stop_client(){
	g_isexit = 1;
	close(g_sock);
	pthread_mutex_unlock(&lock);
    pthread_cond_signal(&queue_send);
	pthread_join(ping_thread_id,NULL);
	pthread_join(core_thread_id,NULL);
}
int createConnect(char* serverip,int port){
	int sockfd, len;
    struct sockaddr_in dest;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
		return -1;
    }
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (inet_aton(serverip, (struct in_addr *) &dest.sin_addr.s_addr) == 0)
    {
        perror(serverip);
        exit(errno);
    }
    if(connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0)
    {
        perror("Connect ");
		return -1;
    }
}
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
void sendhelo(int sockfd,char* drivceId)
{
	pthread_mutex_lock(&lock);
    void* buff = NULL;
    int length = createClientHelo(&buff,CLIENT_TYPE,drivceId,NULL);
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

void sendping(int sockfd)
{	
	pthread_mutex_lock(&lock);
    void* buff = createClientPing(CLIENT_TYPE);
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
void ping_process(void* param)
{
    while(g_isexit == 0)
    {
        int i = 0;
        for(i = 0; i<100; i++)
        {
            if(g_isexit == 0)
                sleep(1);
            else
                return;
        }
        if(g_sock != -1)
	        sendping(g_sock);
    }
}

int message_processer(char* serverip,int port,char* drivceId,char* tempPath,void* ptr)
{
    int isQuit = 0;
    while(isQuit == 0)
    {
        int sockfd, len;
        char bufs[MAXBUF + 1];
        char token[MAXBUF];
        fd_set rfds;
        struct timeval tv;
        int retval, maxfd = -1;
        //bzero(&dest, sizeof(dest));
        //bzero(&token,sizeof(token));
        //strcpy(token,argv[3]);
        //char* tempArgv = malloc(strlen(argv[4]));
        //strcpy(tempArgv,argv[4]);
        //int totokenLength = splitcountx(argv[4],"_");
        //char** totoken_temp = NULL;
        //totoken_temp = malloc(totokenLength);
        //int i =0;
        //for(i=0; i<totokenLength; i++)
        //{
        //    totoken_temp[i] = malloc(64);
        //}
        //totokenLength = split(totoken_temp,tempArgv,"_");
        //free(tempArgv);
        //bzero(&totoken_temp,sizeof(totoken_temp));

        //strcpy(totoken_temp,argv[4]);
        sockfd = createConnect(serverip,port);
        if(sockfd == -1)
        	return -1;
        g_sock = sockfd;
        printf("connect to server...\n");
        sendhelo(sockfd,drivceId);
        //start ping thread
        //thread_struct st;
        //st.sockfd = sockfd;
        //st.isexit = 0;
        //strcpy(st.token,token);
       
        while (1)
        {
            FD_ZERO(&rfds);
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
                if (FD_ISSET(sockfd, &rfds))
                {                	
                    bzero(bufs, MAXBUF + 1);
                    int lockret = pthread_mutex_trylock(&lock);
                    if(lockret == EBUSY)
                    	continue;
                    len = recv(sockfd, bufs, MAXBUF, 0);
                    printf("recv:\n");
                    char* buf = bufs;
                    dump_data(buf,len);
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
                                                             &header,fromdrivceId,
															 messageid,&message,len,tempPath);
                                if(ret < 0)
                                {
                                    printf("has error\n");
                                }
                                //printf("%s,%s,%s",messageid,fromdrivceId,message);
                                if(header.command == MESSAGE_TYPE_TXT){
                                	recvData(messageid,fromdrivceId,message,ptr);
                                }
                                else{
                                	recvFile(messageid,fromdrivceId,message,ptr);
                                }
                                free(message);
                                char* retbuf = createClientHeader(COMMAND_YES,MESSAGE_TYPE_NULL,0x1);
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
                        break;
                    }
                    // free(header);                    
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        close(sockfd);
        //pthread_mutex_init(&st.lock, NULL);
        //st.isexit = 1;
        //pthread_mutex_unlock(&st.lock);
        //pthread_join(thread_id,0);
    }
    return 0;
}


int start_client(char* serverip,int port,char* drivceId,char* tempPath,void* ptr){
   	client_config_t config;
	strcpy(config.serverip,serverip);
	config.port = port;
	strcpy(config.drivceId,drivceId);
	strcpy(config.tempPath,tempPath);
	config.ptr = ptr;
	pthread_mutex_init(&lock,NULL);

    if (pthread_create(&core_thread_id, NULL, (void *) recv_message_thread, &config) != 0)
    {
        printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
               errno, strerror(errno));
		return -1;
    }    
	if(pthread_create(&ping_thread_id,NULL,(void *) ping_thread	,NULL) != 0){
		printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
	   errno, strerror(errno));
		return -1;
	}
	
	if(pthread_create(&send_thread_id,NULL,(void *) send_message_thread,NULL) != 0){
		printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
	   errno, strerror(errno));
		return -1;
	}
	
}
/*
void sendMessage(char* clientMessageid,
							char* message,
							int messagetype,
							list_t* sendtoList){
								
}
*/					
int main(int argc, char **argv)
{
    if (argc <= 3)
    {
        printf("Usage: %s IP Port token send to\n",argv[0]);
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
    
    if(start_client(argv[1],atoi(argv[2]),argv[3],"/tmp",NULL)==-1){
    	printf("create connect error\n");
    	exit(0);	
    }
   	
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
        	user_info_t* uinfo = user_register(username,"");
        	print_userinfo(uinfo);
        	freeUserInfo(uinfo);
			//pthread_mutex_lock(&lock);
			//createClientUserReg(g_sock,CLIENT_TYPE,username);
			//__DEBUG("\n");
			//recvReturn(g_sock);
			//pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"login",5)){
			pthread_mutex_lock(&lock);
			user_info_t* uinfo = user_login(username,"");
			print_userinfo(uinfo);
			freeUserInfo(uinfo);
			//createClientUserLogin(g_sock,1,username);
			//recvReturn(g_sock);
			//pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"find",4)){
        	char fuser[255];
        	sscanf(buf,"%*s%s",fuser);
        	user_info_t* uinfo = find_User(fuser);
        	print_userinfo(uinfo);
        	freeUserInfo(uinfo);
			//pthread_mutex_lock(&lock);
			//createClientUserFindUser(g_sock,1,fuser);
			//recvReturn(g_sock);
			//pthread_mutex_unlock(&lock);
        	continue;
        }
        if(!strncasecmp(buf,"add",3)){
        	char fuser[255];
        	sscanf(buf,"%*s%s",fuser);
        	user_info_t* info = add_Friend(username,fuser);
        	print_userinfo(uinfo);
			freeUserInfo(uinfo);
			//pthread_mutex_lock(&lock);
			//createClientUserAddUser(g_sock,1,username,fuser);
			//recvReturn(g_sock);
			//pthread_mutex_unlock(&lock);
        	continue;
        }
		if(!strncasecmp(buf,"get",3)){
//        	char fuser[255];
//        	sscanf(buf,"%*s%s",fuser);
			//pthread_mutex_lock(&lock);
			//createClientUserAddUser(g_sock,1,username,fuser);
			//createClientUserGetFriends(g_sock,1,username);
			//recvReturn(g_sock);			
//			pthread_mutex_unlock(&lock);
			list_t* list = get_MyFriends(username);
			if(list != NULL){
				for(i=0;i<list->len;i++){
					print_userinfo(list_at(list,i)->val);
				}
			}
			list_destroy(list);
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
			sendMessage(MESSAGE_TYPE_FILE,0,buf,send_list);
		else
			sendMessage(MESSAGE_TYPE_FILE,0,filepath,send_list);
		/*	
        if(isfile == 0)
            len = createClientMessage(g_sock,MESSAGE_TYPE_TXT,1,0,buf,send_list);
        else
            len = createClientMessage(g_sock,MESSAGE_TYPE_FILE,1,0,filepath,send_list);
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
        */

    }
    //pthread_join(thr_id,NULL);
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
