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

#include "util.h"
#include "thread_pool.h"
#include <sys/resource.h>
#include <syslog.h>
#include "log.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <hiredis/hiredis.h>
#include <string.h>
#include <strings.h>
#include "sc.h"
#include "messagestorage.h"
#include "redis_pool.h"
#include "util.h"
#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif

long delytime(int delytime){
	time_t t;
	time(&t); 
	int number = rand() % delytime + 1;
	return t+number;
}

void print_push_info(push_message_info_t* info){
	if(info != NULL){
		printf(">> retrycount = %d\n",info->retrycount);
		printf(">> state = %d\n",info->state);
		printf(">> messagetype = %d\n",info->messagetype);
		printf(">> messageid = %s\n",info->messageid);
		printf(">> submessageid = %s\n",info->submessageid);
		printf(">> to = %s\n",info->to);
		printf(">> orgFileName = %s\n",info->orgFileName);
		printf(">> sendtime = %s\n",info->sendtime);
		printf(">> form = %s\n",info->form);
		printf(">> fromip = %s\n",info->fromip);
		printf(">> timeout = %d\n",info->timeout);
		printf(">> content = %s\n",info->content);				
	}	
}
void print_client_info(client_info_t* info){
	if(info != NULL){		
		printf(">> drivceId = %s\n",info->drivceId);
		printf(">> clienttype = %d\n",info->clienttype);
		printf(">> clinetport = %d\n",info->clinetport);
		printf(">> clientid = %d\n",info->clientid);
		printf(">> serverid = %d\n",info->serverid);
		printf(">> clientip = %s\n",info->clientip);
		printf(">> serverip = %s\n",info->serverip);
		printf(">> logintime = %s\n",info->logintime);			
	}	
}

void changeClientId(char* drivceid,int clientid){
	if(drivceid == NULL)
		return;
	if(clientid<0)
		return;
	int redisId=0;    
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = (redisReply*)redisCommand(redis, "hset %s_content %d",drivceid,clientid);
	freeReplyObject(reply);	
	returnRedis(redisId);
}

int getmessage(int serverid,char* des){
	if(des == NULL)
		return -1;
	int redisId=0;    
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = (redisReply*)redisCommand(redis, "rpop pushlist");	
	if (reply->type == REDIS_REPLY_STRING) {
		if(reply->str != NULL){
			strcpy(des,reply->str);	
			ret = 0;
		}
		else
			ret = 1;  				
	}
    freeReplyObject(reply);	
	returnRedis(redisId);
	return ret;
}
void freePushMessage(push_message_info_t* messageinfo){
	if(messageinfo == NULL)
		return;
	if(messageinfo->content)
		free(messageinfo->content);
	messageinfo->content = NULL;
	free(messageinfo);
	messageinfo = NULL;
}
push_message_info_t* getmessageinfo(char* pushid,push_message_info_t** info_in)
{
	if(pushid == NULL)
		return NULL;
	//if(info == NULL)
	//	return NULL;	
	if(!isHasMessageInfo(pushid))
		return NULL;		
	push_message_info_t* info = *info_in;
	if(info == NULL)
		info = malloc(sizeof(push_message_info_t));
	memset(info,0,sizeof(push_message_info_t));
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	int j = 0;	
	reply = redisCommand(redis,"hgetall %s",pushid);
	if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements-1; j++) {
        	if(strncasecmp(reply->element[j]->str,"submessageid",strlen("submessageid")) == 0){//1
       			strcpy(info->submessageid ,reply->element[j+1]->str);
				j++;	
			}else if(strncasecmp(reply->element[j]->str,"messageid",strlen("messageid")) == 0){//2
       			strcpy(info->messageid ,reply->element[j+1]->str);
				j++;
	        }else if(strncasecmp(reply->element[j]->str,"clientport",strlen("clientport")) == 0){//3
       			info->retrycount = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"sendtime",strlen("sendtime")) == 0){//4
       			strcpy(info->sendtime,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"to",strlen("to")) == 0){//5
       			strcpy(info->to,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"state",strlen("state")) == 0){//6
       			info->state = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"messagetype",strlen("messagetype")) == 0){//7
       			info->messagetype = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"orgFileName",strlen("orgFileName")) == 0){//8
       			strcpy(info->orgFileName,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"form",strlen("form")) == 0){//9
       			strcpy(info->form,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"fromip",strlen("fromip")) == 0){//10
       			strcpy(info->fromip,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"content",strlen("content")) == 0){//11
	        	info->content = malloc(strlen(reply->element[j+1]->str)+1);
	        	memset(info->content,0,strlen(reply->element[j+1]->str)+1);
       			strcpy(info->content,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"timeout",strlen("timeout")) == 0){//12
       			info->timeout = atoi(reply->element[j+1]->str);
				j++;	
	        }	        
        }
    }else{
    	//info->error = 1;
    	//info->errStr = "query error";
    	freePushMessage(info);
    	info = NULL;
    	*info_in = info;
    	returnRedis(redisId);
    	return NULL;
    }
    freeReplyObject(reply);	
	returnRedis(redisId);
	*info_in = info;
	return info;
}
void freeClientInfo(client_info_t* clientinfo){
	if(clientinfo != NULL)
		free(clientinfo);
	clientinfo = NULL;
}
push_message_info_t* putmessageinfo2(char* messageid,char* message,
									char* tochar,
									char* fileName,
									char* newFilename,
									int timeout,
									char* from,
									char* fromip,
									int messagetype,
									int priority,
									push_message_info_t** info_in){
	int redisId=0;
    redisContext* redis = getRedis(&redisId);
    redisReply* reply = NULL;
    if(messageid == NULL || strlen(messageid) == 0){
    	createGUID(messageid);
    }
	char guid[64]={0};
	createGUID(guid);
    char time[255];
    formattime(time,NULL);       
    push_message_info_t* info = *info_in;
    if(info == NULL)
		info = malloc(sizeof(push_message_info_t));
    strcpy(info->submessageid,guid);
    client_info_t* clientinfo = NULL;
    getClientInfo(&clientinfo,tochar); 
    redisAppendCommand(redis,"sadd %s %s",messageid,guid);//13
    redisAppendCommand(redis,"EXPIRE %s %d",messageid,timeout);//14
    if(clientinfo != NULL){
    	if(priority == 0){
	    	redisAppendCommand(redis,"lpush pushlist%d %s",clientinfo->serverid,guid);//1	
    	}
    	else{
    		//TODO Need to improve the delay algorithm
    		long t = delytime((priority*10)*60);
   			redisAppendCommand(redis,"zadd dely_pushlist_set %d %s",t,guid);//1		
	    }    	
    }
    else{
    	redisAppendCommand(redis,"lpush %s_noread %s",tochar,guid);//1    
    }	         	
    redisAppendCommand(redis,"hset %s from %s",guid,from);//2
    strcpy(info->form,from);
    redisAppendCommand(redis,"hset %s to %s",guid,tochar);//3
    strcpy(info->to,tochar);   
    redisAppendCommand(redis,"hset %s fromip %s",guid,fromip);//4
    strcpy(info->fromip,fromip);
    redisAppendCommand(redis,"hset %s state %d",guid,0);//5
    info->state = 0;
    
    if(messagetype == MESSAGE_TYPE_TXT)
    {
        redisAppendCommand(redis,"hset %s content %s",guid,message);//6
        info->content = malloc(strlen(message)+1);
        memset(info->content,0,strlen(message)+1);
		memcpy(info->content,message,strlen(message));
    }
    else
    {        
        redisAppendCommand(redis,"hset %s orgFileName %s",guid,fileName);//6
		strcpy(info->orgFileName,fileName);
        redisAppendCommand(redis,"hset %s content %s",guid,newFilename);//12
        info->content = malloc(strlen(newFilename)+1);
        memset(info->content,0,strlen(message)+1);
		memcpy(info->content,newFilename,strlen(newFilename));
    }
    redisAppendCommand(redis,"hset %s sendtime %s",guid,time);//7
    strcpy(info->sendtime,time);
    redisAppendCommand(redis,"hset %s retrycount %d",guid,0);//8
    info->retrycount = 0;
    redisAppendCommand(redis,"hset %s messagetype %d",guid,messagetype);//9
    info->messagetype = messagetype;
    redisAppendCommand(redis,"hset %s submessageid %s",guid,guid);//10
    strcpy(info->submessageid,guid);
    redisAppendCommand(redis,"hset %s messageid %d",guid,messageid);//13
	strcpy(info->messageid,messageid);
    redisAppendCommand(redis,"EXPIRE %s %d",guid,timeout);//11
    info->timeout = timeout;
    int x=0;				
	for(x=0;x<14;x++){
		redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);//1	
	} 
    if(messagetype != MESSAGE_TYPE_TXT)
    {
        redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);//11
    }
	freeClientInfo(clientinfo);
	returnRedis(redisId);		
	*info_in = info;						
	return info;									
}
push_message_info_t* putmessageinfo(char* message,
									char* tochar,
									char* fileName,
									char* newFilename,
									int timeout,
									CLIENT_HEADER* header,
									CLIENT* client,
									int priority,
									push_message_info_t** info_in){
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
    redisReply* reply = NULL;
	char guid[64]={0};
	createGUID(guid);
    char time[255];
    formattime(time,NULL);       
    push_message_info_t* info = *info_in;
    if(info == NULL)
		info = malloc(sizeof(push_message_info_t));
    strcpy(info->submessageid,guid);
    client_info_t* clientinfo = NULL;
    getClientInfo(&clientinfo,tochar); 
    if(clientinfo != NULL){
    	if(priority == 0){
	    	redisAppendCommand(redis,"lpush pushlist%d %s",clientinfo->serverid,guid);//1	
    	}
    	else{
    		//TODO Need to improve the delay algorithm
    		long t = delytime((priority*10)*60);
   			redisAppendCommand(redis,"zadd dely_pushlist_set %d %s",t,guid);//1		
	    }    	
    }
    else{
    	redisAppendCommand(redis,"lpush %s_noread %s",tochar,guid);//1    
    }	         	
    redisAppendCommand(redis,"hset %s from %s",guid,header->clienttoken);//2
    strcpy(info->form,header->clienttoken);
    redisAppendCommand(redis,"hset %s to %s",guid,tochar);//3
    strcpy(info->to,tochar);   
    redisAppendCommand(redis,"hset %s fromip %s",guid,inet_ntoa(client->addr.sin_addr));//4
    strcpy(info->fromip,inet_ntoa(client->addr.sin_addr));
    redisAppendCommand(redis,"hset %s state %d",guid,0);//5
    info->state = 0;
    
    if(header->messagetype == MESSAGE_TYPE_TXT)
    {
        redisAppendCommand(redis,"hset %s content %s",guid,message);//6
        info->content = malloc(strlen(message)+1);
        memset(info->content,0,strlen(message)+1);
		memcpy(info->content,message,strlen(message));
    }
    else
    {        
        redisAppendCommand(redis,"hset %s orgFileName %s",guid,fileName);//6
		strcpy(info->orgFileName,fileName);
        redisAppendCommand(redis,"hset %s content %s",guid,newFilename);//12
        info->content = malloc(strlen(newFilename)+1);
        memset(info->content,0,strlen(message)+1);
		memcpy(info->content,newFilename,strlen(newFilename));
    }
    redisAppendCommand(redis,"hset %s sendtime %s",guid,time);//7
    strcpy(info->sendtime,time);
    redisAppendCommand(redis,"hset %s retrycount %d",guid,0);//8
    info->retrycount = 0;
    redisAppendCommand(redis,"hset %s messagetype %d",guid,header->messagetype);//9
    info->messagetype = header->messagetype;
    //TODO
    redisAppendCommand(redis,"hset %s submessageid %s",guid,guid);//10
    strcpy(info->submessageid,guid);
    redisAppendCommand(redis,"EXPIRE %s %d",guid,timeout);//11
    info->timeout = timeout;
    int x=0;				
	for(x=0;x<11;x++){
		redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);//1	
	} 
    if(header->messagetype != MESSAGE_TYPE_TXT)
    {
        redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);//11
    }
	freeClientInfo(clientinfo);
	returnRedis(redisId);		
	*info_in = info;						
	return info;
}
/**********************
 *
 * hash deviceid_content
 {
 	clientip: xxx.xxx.xxx.xxx
	clientport:xxxx
	logintime:xxxx-xx-xx xx:xx:xx
	devicetype:0(android),1(IOS),2(Web),3(WP),4(other)
	serverip: xxx.xxx.xxx.xxx
 }
 * score set xxx.xxx.xxx.xxx
 {
	deviceid,port
 }
 *
 ***********************/
 client_info_t* newClient(CLIENT* client,
 					int serverid,
					char* serverip,
					char* drivceId,
					int clienttype,
					client_info_t** info){											
	return putclientinfo(client,serverid,serverip,drivceId,clienttype,info);
}
client_info_t* putclientinfo(CLIENT* client,
					int serverid,
					char* serverip,
					char* drivceId,
					int clienttype,
					client_info_t** info_in)
{
    char time[255];
    int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
    formattime(time,NULL);
    client_info_t* info = *info_in;
    if(info == NULL)
    	info = malloc(sizeof(client_info_t));
   	memset(info,0,sizeof(client_info_t));
    redisAppendCommand(redis,"hset %s_content clientip %s",drivceId,inet_ntoa(client->addr.sin_addr));//1
    redisAppendCommand(redis,"hset %s_content clientport %d",drivceId,client->addr.sin_port);//2
    redisAppendCommand(redis,"hset %s_content logintime %s",drivceId,time);//3
    redisAppendCommand(redis,"hset %s_content serverip %s",drivceId,serverip);//4
    ///////////////////////
    redisAppendCommand(redis,"hset %s_content clienttype %d",drivceId,clienttype);//5
    //TODO need add devicetype
    redisAppendCommand(redis,"zdd %s %d %s",inet_ntoa(client->addr.sin_addr),client->addr.sin_port,drivceId);//6
    redisAppendCommand(redis,"lpush userlist %s",drivceId);//7
    redisAppendCommand(redis,"hset %s_content serverid %d",drivceId,serverid);//8
    redisAppendCommand(redis,"hset %s_content clientid %d",drivceId,client->clientId);//9
    int x = 0;
    for(x=0; x<9; x++)
    {
        redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);
    }
    //strcpy(client->drivceId,drivceId);    
	if(info != NULL){
		strcpy(info->clientip,inet_ntoa(client->addr.sin_addr));
		info->clinetport = client->addr.sin_port;
		strcpy(info->logintime,time);
		strcpy(info->serverip,serverip);
		info->clienttype = clienttype;
		strcpy(info->drivceId,drivceId);		
		info->clientid = client->clientId;
		info->serverid = serverid;
		info->error = 0;
	}    
	returnRedis(redisId);
	*info_in = info;
	return info;
}
/*****************************
     *
     *SortedSet xxx.xxx.xxx.xxx
      zrangbyscore
      del drivce_conetent
      ZREMRANGEBYSCORE

 ********************/
void removeClient(CLIENT* client){
	if(client == NULL)
		return ;
	if(client->drivceId == NULL || strlen(client->drivceId) == NULL)
		return;
	int redisId = 0,i=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;	
	redisAppendCommand(redis,"del %s_content",client->drivceId);//1
	redisAppendCommand(redis,"lrem userlistonline 0 %s",client->drivceId);//2
	redisAppendCommand(redis,"ZREMRANGEBYSCORE %s %d %d",inet_ntoa(client->addr.sin_addr),client->addr.sin_port,client->addr.sin_port);//3
	for(i=0;i<3;i++){
		redisGetReply(redis,(void**)&reply);
 		freeReplyObject(reply);
	}	
 	returnRedis(redisId);
}
int isHasMessageInfo(char* messageid){
	if(messageid == NULL)
		return -1;
	int ret = 0;
	int redisId = 0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	
	reply = redisCommand(redis,"exists %s",messageid);
	if (reply->type == REDIS_REPLY_INTEGER) {
		if(reply->integer == 1)
			ret = 1;
		else
			ret = 0;
	}
    freeReplyObject(reply);	
	returnRedis(redisId);
	return ret;
}
int isClientOnline(char* drivceId){
	if(drivceId == NULL)
		return -1;
	int ret = 0;
	int redisId = 0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	
	reply = redisCommand(redis,"exists %s_content",drivceId);
	if (reply->type == REDIS_REPLY_INTEGER) {
		//printf("exists %s_content %d,%s\n",drivceId,reply->integer,reply->str);
		if(reply->integer == 1)
			ret = 1;
		else
			ret = 0;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);
	return ret;
}
int getNoReadMessageSize(char* drivceId){
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	int len = 0;
	reply = (redisReply*)redisCommand(redis, "llen %s_noread",drivceId);
    if( NULL == reply){
    	returnRedis(redisId);	
		return 0;  
    }
	if (reply->type == REDIS_REPLY_INTEGER) {   
		len = reply->integer;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);
	return len;
}
void saveInNoread(char* drivceId,char* messageid){
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = (redisReply*)redisCommand(redis,"lpush %s_noread %s",drivceId,messageid);    
	freeReplyObject(reply);	
	returnRedis(redisId);
}
void saveInPushlist(char* messageid,int serverid){
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = (redisReply*)redisCommand(redis,"lpush pushlist%d %s",serverid,messageid);    
	freeReplyObject(reply);	
	returnRedis(redisId);
}
int getNextNoReadMessageId(char* drivceId,char* messageid){
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	int len = 0;
	reply = (redisReply*)redisCommand(redis, "rpop %s_noread",drivceId);
    if( NULL == reply){
    	returnRedis(redisId);	
    	return 0;  
    }		
	if (reply->type == REDIS_REPLY_STRING) {   
		strncpy(messageid,reply->str,reply->len);
		len = reply->len;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);
	return len;
}
void messageIsSendOK2(push_message_info_t* messageinfo,int serverid){
	if(messageinfo == NULL)
		return;	
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply;
    redisAppendCommand(redis,"hset %s state 1",messageinfo->submessageid); //1
    redisAppendCommand(redis,"lrem pushlist%d 0 %s",serverid,messageinfo->submessageid); //2
    redisAppendCommand(redis,"lrem %s_noread 0 %s",messageinfo->to,messageinfo->submessageid);//3
    redisAppendCommand(redis,"lpush %s_read %s",messageinfo->to,messageinfo->submessageid);//4
    int x = 0;
    for(x=0;x<4;x++){
    	redisGetReply(redis,(void**)&reply);
    	freeReplyObject(reply);
    }    
	returnRedis(redisId);	
}
void messageIsSendOK(push_message_info_t* messageinfo,client_info_t	* clientinfo){
	if(messageinfo == NULL)
		return;
	if(clientinfo == NULL)
		return;
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply;
    redisAppendCommand(redis,"hset %s state 1",messageinfo->submessageid); //1
    redisAppendCommand(redis,"lrem pushlist%d 0 %s",clientinfo->serverid,messageinfo->submessageid); //2
    redisAppendCommand(redis,"lrem %s_noread 0 %s",messageinfo->to,messageinfo->submessageid);//3
    redisAppendCommand(redis,"lpush %s_read %s",messageinfo->to,messageinfo->submessageid);//4
    int x = 0;
    for(x=0;x<4;x++){
    	redisGetReply(redis,(void**)&reply);
    	freeReplyObject(reply);
    }    
	returnRedis(redisId);	
}
int getNextMessageId(int serverid,char* messageid){
	if(messageid == NULL){
		return 0;
	}
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	int len = 0;
	
	memset(messageid,0,255);
	reply = (redisReply*)redisCommand(redis, "rpop pushlist%d",serverid);
    if( NULL == reply){
    	returnRedis(redisId);
		return 0;      	
    }
 	if (reply->type == REDIS_REPLY_STRING) {   
		strncpy(messageid,reply->str,reply->len);
		len = reply->len;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);
	return len;
}
/*
int removeMessageFromNoRead(int serverid,char*drviceid,char* messageid){
	reply = (redisReply*)redisCommand(redis, "lrem pushlist%d 0 %s",serverid,messageid);
}
*/
client_info_t* getClientInfo(client_info_t** info_in,char* drivceId){
	if(drivceId == NULL)
		return NULL;
	client_info_t* info = *info_in;	
	//	return NULL;
	if(isClientOnline(drivceId) <= 0){
		return NULL;	
	}
	if(info == NULL)
		info = malloc(sizeof(client_info_t));
	memset(info,0,sizeof(client_info_t));
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;

	int j = 0;
	reply = redisCommand(redis,"hgetall %s_content",drivceId);
	if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements-1; j++) {
        	if(strncasecmp(reply->element[j]->str,"clientip",8) == 0){
       			strcpy(info->clientip ,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"clientport",strlen("clientport")) == 0){
       			info->clinetport = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"logintime",strlen("logintime")) == 0){
       			strcpy(info->logintime,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"serverip",strlen("serverip")) == 0){
       			strcpy(info->serverip,reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"clienttype",strlen("clienttype")) == 0){
       			info->clienttype = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"serverid",strlen("serverid")) == 0){
       			info->serverid = atoi(reply->element[j+1]->str);
				j++;	
	        }else if(strncasecmp(reply->element[j]->str,"clientid",strlen("clientid")) == 0){
       			info->clientid = atoi(reply->element[j+1]->str);
				j++;	
	        }
        }
        strcpy(info->drivceId,drivceId);
    }else{
    	//info->error = 1;
    	//info->errStr = "query error";
    	freeClientInfo(info);
    	info = NULL;
    	returnRedis(redisId);
    	return NULL;
    }
    freeReplyObject(reply);	
	returnRedis(redisId);
	*info_in = info; 
	return info;
}
int getClientDrivceIdFromMessage(char* messageid,char* drivceId){
	int redisId=0;        
	int len = 0;
	if(messageid == NULL)
		return 0;
	redisContext* redis = getRedis(&redisId);
	redisReply* reply = redisCommand(redis,"hget %s to",messageid); 
	if(reply != NULL && reply->type == REDIS_REPLY_STRING){
		strncpy(drivceId,reply->str,reply->len);
		len = 1;			
	}
	else{
		len = 0;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);
	return len;
}
/*

WATCH dely_pushlist_set
element=zrangebyscore dely_pushlist_set -inf 0 limit 0 1
MULTI
    ZREM dely_pushlist_set element
EXEC

*/
int getDelyMessage(char* messageid){
	int len = 0;
	if(messageid == NULL)
		return 0;
	int redisId=0;    
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	memset(messageid,0,255);
	time_t t;
	time(&t); 
	reply = redisCommand(redis,"watch dely_pushlist_set"); //1
 	freeReplyObject(reply);	
	reply = (redisReply*)redisCommand(redis, "zrangebyscore dely_pushlist_set -inf %d limit 0 1",t);
	if (reply->type == REDIS_REPLY_STRING) {  
		strncpy(messageid,reply->str,reply->len);
		len = 1;
	}else{
		returnRedis(redisId);
		return 0;
	}
	freeReplyObject(reply);	
	
	redisAppendCommand(redis, "MULTI");
	redisAppendCommand(redis, "zrem dely_pushlist_set %s",messageid);
	redisAppendCommand(redis, "EXEC");
	
    redisGetReply(redis,(void**)&reply);
	freeReplyObject(reply);	
	redisGetReply(redis,(void**)&reply);
	freeReplyObject(reply);	
	redisGetReply(redis,(void**)&reply);
	//if(reply == NULL || reply->type==)
	//TODO return null, if redis return null multi-bulk reply 
	freeReplyObject(reply);	
	returnRedis(redisId);
	return len;
}

