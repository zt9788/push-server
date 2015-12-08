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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "extended_storage.h"
//#include "messagestorage.h"
#include "extended_protocol.h"
#include "parse_command.h"
#include "client.h"

extern list_t* sendlist;
extern pthread_mutex_t send_lock;
extern pthread_cond_t  queue_send;
extern int g_sock;
extern pthread_mutex_t lock;


list_t*/*<user_info_t>*/ get_MyFriends(char* username){
	
	pthread_mutex_lock(&lock);
	createClientUserGetFriends(g_sock,1,username);
	char buf[MAXBUF]={0};
	server_header_2_t header;
	char* bufs = parseServerHeader(buf,&header);
	int ret = recv(g_sock,bufs,MAXBUF,0);
	list_t* list = NULL;
 	parseServerUserGetUser(g_sock,bufs,&list);	
	pthread_mutex_unlock(&lock);	
	if(list != NULL)
		return list;
	else
		return NULL;
}

user_info_t* add_Friend(char* username,char* friendName){
	pthread_mutex_lock(&lock);
	createClientUserAddUser(g_sock,CLIENT_TYPE,username,friendName);
	char buf[MAXBUF]={0};
	server_header_2_t header;
	char* bufs = parseServerHeader(buf,&header);
	int ret = recv(g_sock,bufs,MAXBUF,0);
	user_info_t* uinfo = NULL;
 	parseServerUserAddUser(g_sock,bufs,&uinfo);	
	pthread_mutex_unlock(&lock);	
	if(uinfo != NULL)
		return uinfo;
	else
		return NULL;		
}
user_info_t* user_login(char* userName,
			char* password/* the password is nothing to do*/){
	pthread_mutex_lock(&lock);
	createClientUserLogin(g_sock,CLIENT_TYPE,userName);
	char buf[MAXBUF]={0};
	server_header_2_t header;
	char* bufs = parseServerHeader(buf,&header);
	int ret = recv(g_sock,bufs,MAXBUF,0);
	int isSucess = 0;
	user_info_t* uinfo = parseServerUserLogin(g_sock,bufs,&isSucess);	
	pthread_mutex_unlock(&lock);	
	if(isSucess)
		return uinfo;
	else
		return NULL;								
}
user_info_t* user_register(char* userName,
			char* password/* the password is nothing to do*/){
	pthread_mutex_lock(&lock);
	createClientUserReg(g_sock,CLIENT_TYPE,userName);
	char buf[MAXBUF]={0};
	server_header_2_t header;
	char* bufs = parseServerHeader(buf,&header);
	int ret = recv(g_sock,bufs,MAXBUF,0);
	int isSucess = 0;
	user_info_t* uinfo = parseServerUserReg(g_sock,bufs,&isSucess);	
	pthread_mutex_unlock(&lock);	
	if(isSucess)
		return uinfo;
	else
		return NULL;			
					
}
user_info_t* find_User(char* userName){
	pthread_mutex_lock(&lock);
	createClientUserFindUser(g_sock,CLIENT_TYPE,userName);
	char buf[MAXBUF]={0};
	server_header_2_t header;
	char* bufs = parseServerHeader(buf,&header);
	int ret = recv(g_sock,bufs,MAXBUF,0);
	int isSucess = 0;
	user_info_t* uinfo = parseServerUserFindUser(g_sock,bufs,&isSucess);	
	pthread_mutex_unlock(&lock);	
	if(isSucess)
		return uinfo;
	else
		return NULL;		
}
void recvData(char* serverMessageId,char* fromDrivceId,char* message,void* data){
	printf(">>--------------------<<\n");
	printf("from:%s\n",serverMessageId);
	printf("fromdrivceId:%s\n",fromDrivceId);
	printf("message:%s\n",message);
	printf(">>--------------------<<\n");
}

void recvFile(char* serverMessageId,char* fromDrivceId,char* fileName,void* ptr){
	//...
}

void sendMessageResponse(char* clientMessageid,char* serverMessageid, int responseCode,
			char* error,void* data){
	
				
}

void sendMessage(char* clientMessageid,int delytime,
							char* message,
							int messagetype,
							list_t* sendtoList){	
	send_message_t* ms = malloc(sizeof(send_message_t));
	ms->message = malloc(strlen(message));
	strcpy(ms->message,message);
	strcpy(ms->messageid,clientMessageid);
	ms->messagetype = messagetype;
	ms->delytime = delytime;
	ms->sendto = list_new();
	ms->sendto->free = free;
	int i = 0;
	for(i=0;i<sendtoList->len;i++){
		list_node_t* node = list_at(sendtoList,i);
		char* str = malloc(strlen(node->val));
		list_lpush(ms->sendto,list_node_new(str));
	}
	list_lpush(sendlist,list_node_new(ms));
	pthread_cond_signal(&queue_send);
	pthread_mutex_unlock(&send_lock);									
}

void* send_message_thread(void* args){
	//when in android ,args = env	
	send_message_processer(args);
}

void* recv_message_thread(void* args){
	//ptr = env in android
	client_config_t* config = args;
	message_processer(config->serverip,config->port,config->drivceId,config->tempPath,config->ptr);
}

void* ping_thread(void* args){
	//ptr = env in android
	ping_process(args);
	
}