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
#include "parse_command.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <stdio.h>
#include <time.h>
#include "messagestorage.h"
#include "util.h"
#define MAX_DRIVCEID_LENGTH 64

#define __DEBUG(format, ...) printf("FILE: "__FILE__", LINE: %d: "format"/n", __LINE__, ##__VA_ARGS__)
int createClientMessage(int sock,unsigned	char messsagetype,unsigned	char clienttype,
						short delytime,
						char* contentOrFileName,list_t *sendto){
	client_header_2_t* header = createClientHeader(COMMAND_MESSAGE,messsagetype,clienttype);
	int i = 0;
	int length = sizeof(client_header_2_t);	
	char* tempBufhead = malloc(MAX_DRIVCEID_LENGTH*sendto->len+sizeof(uint16_t)*(sendto->len+1));
	int templenth = 0;
	void* tempBuf = tempBufhead;
	
	*(uint16_t*)tempBuf = htons(sendto->len);
	tempBuf += sizeof(uint16_t);
	templenth += sizeof(uint16_t);
	list_node_t *last = NULL;
		
	while((last=list_lpop(sendto))!= NULL){
		*(uint16_t*)tempBuf=htons(strlen(last->val));
		tempBuf += sizeof(uint16_t);
		memcpy(tempBuf,last->val,strlen(last->val));
		tempBuf += strlen(last->val);		
		
		templenth += sizeof(uint16_t);
		templenth += strlen(last->val);
		
		free(last);
	}
	
	//TODO file command 
	length += sizeof(uint16_t)+templenth;
	char* filename = NULL;
	if(messsagetype == MESSAGE_TYPE_TXT){
		length += sizeof(uint16_t)+strlen(contentOrFileName);
	}
	else{
		filename = getFileName(contentOrFileName);
		length += sizeof(uint16_t)+strlen(filename)+sizeof(uint16_t);
	}	
	header->total = length;
	void* retbuf = malloc(length);
	char* buf = retbuf;
	memcpy(buf,header,sizeof(client_header_2_t));
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(delytime);
	buf += sizeof(uint16_t);	
	memcpy(buf,tempBufhead,templenth);
	buf += templenth;
	if(messsagetype == MESSAGE_TYPE_TXT){
		*(uint16_t*)buf = htons(strlen(contentOrFileName));
		buf += sizeof(uint16_t);
		memcpy(buf,contentOrFileName,strlen(contentOrFileName));	
	}
	else{		
		*(uint16_t*)buf = htons(strlen(filename));
		buf += sizeof(uint16_t);
		memcpy(buf,filename,strlen(filename));
		buf += strlen(filename);
		*(uint32_t*)buf = get_file_size(contentOrFileName);
	}
	
	free(tempBufhead);
	int sendlen = 0;
	int sumlen = 0;
	printf("start send\n");
	//TODO
	dump_data(retbuf,length);
	do{
		if((length-sumlen) > MAXBUF)
			sendlen = send(sock,retbuf,MAXBUF,0);	
		else
			sendlen = send(sock,retbuf,length-sumlen,0);	
		sumlen += sendlen;	
	}while(sumlen<length);
	
	free(retbuf);
	return sendlen;
}
int parseClientMessage(int sockfd,void* bufs,client_header_2_t* header,char* driveceId,
		char* token,int recvlen,char* fromip,char* tempPath,char* outMessageid){
	char* buf = bufs;
	int i = 0;
	//char fileName[255]= {0};
    char newFilename[500]= {0};    
	int delytime = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	int sendtocount = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	void* contentOrFileName = NULL;
	char** sendto = (char**)malloc(sizeof(char*)*sendtocount);
	int residueLen = recvlen-sizeof(uint16_t)*2;
	if(sendtocount > 0 ){
		for(i=0;i<sendtocount;i++){
			int len = ntohs(*(uint16_t*)buf);
			sendto[i] = malloc(len+1);
			memset(sendto[i],0,len+1);
			buf += sizeof(uint16_t);
			strncpy(sendto[i],buf,len);
			buf += len;
			residueLen = residueLen - (sizeof(uint16_t) + len);
		}
	}
	int recvlens = recvlen;
	char recvBuf[MAXBUF]={0};
	if(header->messagetype == MESSAGE_TYPE_TXT){
		int length = ntohs(*(uint16_t*)buf);       
		buf += sizeof(uint16_t);
		residueLen -= sizeof(uint16_t);
		contentOrFileName = malloc(length+1);
		memset(contentOrFileName,0,length+1);
		int templen = -1;
		if(recvlen >= header->total-sizeof(client_header_2_t)){
			strncpy(contentOrFileName,buf,length);		
		}		
		else{
			memcpy(contentOrFileName,buf,residueLen);	
			while(header->total>recvlens){			
				int ret = -1;
				if((header->total-recvlens)>MAXBUF){							
					ret = recv(sockfd,recvBuf,MAXBUF,0);				
				}else{							
					ret = recv(sockfd,recvBuf,header->total-recvlens,0);
				}
				if(ret<=0){
					free(contentOrFileName);
					return -1;
				}
				memcpy(contentOrFileName,recvBuf,ret);
				recvlens += ret;
			}
		}
	}else{	
		//TODO				
		char tempStr[500]= {0};		
		int length = ntohs(*(uint16_t*)buf);
		buf += sizeof(uint16_t);
		contentOrFileName = malloc(length);
		memcpy(contentOrFileName,buf,length);
		
		char* fileName = getFileName(contentOrFileName);
  		sprintf(tempStr,"%s",fileName);
        sprintf(tempStr,"%s%s",tempStr,time);
        createMd5(tempStr,newFilename);
        int filesize = ntohl(*(uint32_t*)buf);
        *buf += sizeof(uint32_t);
        char fullpath[500]={0};
        createfullpath(tempPath,newFilename,fullpath);
        FILE * flogx=fopen(fullpath,"a");
    	if (NULL==flogx){
	    	free(contentOrFileName);
	    	return -1;
	    }    		
	    residueLen = residueLen -(sizeof(uint16_t)+sizeof(uint32_t)+length);
        fwrite(buf,recvlens-residueLen,1,flogx);
		while(header->total>recvlens){			
			int ret = -1;
			if((header->total-recvlens)>MAXBUF){							
				ret = recv(sockfd,recvBuf,MAXBUF,0);				
			}else{							
				ret = recv(sockfd,recvBuf,header->total-recvlens,0);
			}
			if(ret<=0){
				free(contentOrFileName);
				fclose(flogx);
				return -1;
			}
			fwrite(recvBuf,ret,1,flogx);
			recvlens += ret;
		}
		fclose(flogx);
	}
	//char messageid[64]={0};
	createGUID(outMessageid);
	for(i=0;i<sendtocount;i++){
		push_message_info_t* info = NULL;
#ifndef CLIENTMAKE	
		//TODO have no dely time to save in redis
		printf("need to save user %s\n",sendto[i]);
		putmessageinfo2(outMessageid,contentOrFileName,sendto[i],contentOrFileName,
					newFilename,240,driveceId,
					fromip,header->messagetype,0,&info);
		if(info != NULL){
			freePushMessage(info);	
		}
#endif				
		free(sendto[i]);
	}
	free(sendto);
	if(contentOrFileName)
		free(contentOrFileName);
	
	return 0;
}
void* createClientPing(unsigned	char clienttype	){
	client_header_2_t* header = createClientHeader(COMMAND_PING,MESSAGE_TYPE_NULL,clienttype);
	header->total = sizeof(client_header_2_t);
	return header;
}
void* createServerPing(int serverid	){
	server_header_2_t* header = createServerHeader(serverid,COMMAND_PING,MESSAGE_TYPE_NULL);//createClientHeader(COMMAND_PING,MESSAGE_TYPE_TXT,clienttype);
	header->total = sizeof(server_header_2_t);
	return header;
}

int  createClientHelo(void** retbuffer,unsigned	char clienttype,char* drivceId,char* iostoken){
	client_header_2_t* header = createClientHeader(COMMAND_HELO,MESSAGE_TYPE_TXT,clienttype);
	void* retbuf = NULL;
	int length = sizeof(client_header_2_t)+sizeof(uint16_t)+strlen(drivceId);
	if(clienttype == CLIENT_TYPE_IOS){	
		if(iostoken != NULL && strlen(iostoken) > 0)
			length += sizeof(uint16_t)+strlen(iostoken);
		else
			length += sizeof(uint16_t);
	}
	header->total = length;
	retbuf = malloc(length);
	char* buf = retbuf;
	memcpy(buf,header,sizeof(client_header_2_t));
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(strlen(drivceId));//sizeof(uint16_t));
	buf += sizeof(uint16_t);
	memcpy(buf,drivceId,strlen(drivceId));
	if(clienttype == CLIENT_TYPE_IOS){		
		buf += strlen(drivceId);
		*(uint32_t*)buf = htonl(sizeof(uint32_t));
		buf += sizeof(uint32_t);
		memcpy(buf,iostoken,strlen(iostoken));
	}	
	free(header);
	*retbuffer = retbuf;
	//return retbuf;
	return length;
}

void* parseServerHeader(void* bufs,server_header_2_t* header){
	if(bufs == NULL)
		return NULL;
	char* buf = bufs;
	//if(strlen(buf) < sizeof(server_header_2_t))
	//	return NULL;
	memcpy(header,buf,sizeof(server_header_2_t));
	buf += sizeof(server_header_2_t);
	return buf;
}


void* parseClientHeader(void* bufs,client_header_2_t* header){
	if(bufs == NULL)
		return NULL;
	//char* buf = bufs;
	//if(strlen(buf) < sizeof(client_header_2_t))
	//	return NULL;
	memcpy(header,bufs,sizeof(client_header_2_t));
	bufs += sizeof(client_header_2_t);
	return bufs;
}

int parseClientHelo(void* bufs,char* drivceId,char* iostoken,int clienttype){
 	unsigned char* buf = bufs;
	uint16_t length = ntohs(*(uint16_t*)buf);
	//char* drivceId = malloc(length);
	memcpy(drivceId,buf+sizeof(uint16_t),length);
	buf+=sizeof(uint16_t);
	if(clienttype == CLIENT_TYPE_IOS){
		uint16_t length2 = ntohs(*(uint16_t*)buf);
		if(length2 != 0)
			memcpy(iostoken,buf+sizeof(uint16_t),length);
		return 2;
	}
	return 1;	
}
void* parseServerHelo(void* buf,server_config_to_client_t* config){
	if(config == NULL)
		return NULL;
	memcpy(config,buf,sizeof(server_config_to_client_t));
	return config;
}

void* createServerHelo(int serverid,unsigned char isOpenMessageResponse,unsigned char isOpenFileSocket,unsigned	char isOpenPingResponse){
	server_header_2_t* header = createServerHeader(serverid,COMMAND_HELO,MESSAGE_TYPE_TXT);
	server_config_to_client_t clientconfig;
	memset(&clientconfig,0,sizeof(server_config_to_client_t));
	createServer2ClientConfig(&clientconfig,isOpenMessageResponse,isOpenFileSocket,isOpenPingResponse);
	void* buf = malloc(sizeof(server_header_2_t)+sizeof(server_config_to_client_t));
	header->total = sizeof(server_header_2_t)+sizeof(server_config_to_client_t);
	
	memcpy(buf,header,sizeof(server_header_2_t));
	memcpy(buf+sizeof(server_header_2_t),&clientconfig,sizeof(server_config_to_client_t));
	free(header);
	return buf;
}



server_header_2_t* createServerHeader(int serverid,unsigned char command,unsigned char messagetype){
	server_header_2_t * header = malloc(sizeof(server_header_2_t));
	header->command = command;
	header->serverid = serverid;
	header->messagetype = messagetype;	
	header->total = sizeof(server_header_2_t);	
	return header;	
}
client_header_2_t* createClientHeader(unsigned char command,unsigned char messagetype,unsigned char clienttype){
	client_header_2_t* header = malloc(sizeof(client_header_2_t));
	header->command = command;
	header->clienttype = clienttype;
	header->messagetype = messagetype;
	return header;
}
server_config_to_client_t* createServer2ClientConfig(server_config_to_client_t* config
	,unsigned char isOpenMessageResponse,unsigned	char isOpenFileSocket,unsigned	char isOpenPingResponse){
	if(config == NULL)
		return NULL;
	config->isOpenMessageResponse=isOpenMessageResponse;
	config->isOpenFileSocket=isOpenFileSocket;
	config->isOpenPingResponse=isOpenPingResponse;
	return config;	
}
void* createServerBuffforMessage(void* bufs,
					int serverid,
					push_message_info_t* info,
					char* tmpPath){
	char* buf = bufs;
	if(info->messagetype == MESSAGE_TYPE_TXT){
		//memcpy(buf,htons(strlen(info->content)),sizeof(uint16_t));
		*(uint16_t*)buf = htons(strlen(info->content));
		buf += sizeof(uint16_t);
		memcpy(buf,info->content,strlen(info->content));
	}else{
		//memcpy(buf,htons(strlen(info->orgFileName)),sizeof(uint16_t));
		*(uint16_t*)buf = htons(strlen(info->orgFileName));
		buf += sizeof(uint16_t);
		memcpy(buf,info->orgFileName,strlen(info->orgFileName));
		buf += strlen(info->orgFileName);
		char fullpath[500]={0};
		createfullpath(tmpPath,info->content,fullpath);
		unsigned long filesize = get_file_size(fullpath);
		//memcpy(buf,htonl(filesize),sizeof(uint32_t));
		*(uint32_t*)buf = htonl(filesize);
		buf += sizeof(uint32_t);//TODO max send sizeof(uint32_t)
		int ret = 0;
		unsigned char* tempBuf = readtofile(tmpPath,info->content,&ret);		
		memcpy(buf,tempBuf,ret);
		free(tempBuf);
	}
	return buf;
}
//TODO
int parseServerMessage(int sockfd,void* bufs,server_header_2_t* header,
		char* fromdriveceId,char* messageid,char** content,int recvlen,char* tempPath){
	char* buf = bufs;
	int i = 0;
	
	int residueLen = recvlen;
	//char fileName[255]= {0};
    char newFilename[500]= {0};    
    //sendtime
	int sendtime = ntohl(*(uint32_t*)buf);
	buf += sizeof(uint32_t);
	residueLen -= sizeof(uint32_t);
	//form
	int fromlength = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	strncpy(fromdriveceId,buf,fromlength);
	buf += fromlength;
	residueLen -= sizeof(uint16_t) - fromlength;
	//messsageid
	int messagelength = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	strncpy(messageid,buf,messagelength);
	buf += messagelength;
	residueLen -= sizeof(uint16_t) - messagelength;
	//content
	void* contentOrFileName = NULL;	
	int recvlens = recvlen;
	char recvBuf[MAXBUF]={0};
	if(header->messagetype == MESSAGE_TYPE_TXT){
		int length = ntohs(*(uint16_t*)buf);
		buf += sizeof(uint16_t);
		residueLen -= sizeof(uint16_t);
		contentOrFileName = malloc(length);
		int templen = -1;
		if(recvlen >= header->total-sizeof(client_header_2_t)){
			memcpy(contentOrFileName,buf,length);
			*content = contentOrFileName;	
		}		
		else{
			memcpy(contentOrFileName,buf,residueLen);	
			while(header->total>recvlens){			
				int ret = -1;
				if((header->total-recvlens)>MAXBUF){							
					ret = recv(sockfd,recvBuf,MAXBUF,0);				
				}else{							
					ret = recv(sockfd,recvBuf,header->total-recvlens,0);
				}
				if(ret<=0){
					free(contentOrFileName);
					return -1;
				}
				memcpy(contentOrFileName,recvBuf,ret);
				recvlens += ret;
			}
		}
	}else{	
		//TODO				
		char tempStr[500]= {0};		
		int length = ntohs(*(uint16_t*)buf);
		buf += sizeof(uint16_t);
		contentOrFileName = malloc(length);
		memcpy(contentOrFileName,buf,length);
		
		char* fileName = getFileName(contentOrFileName);
  		sprintf(tempStr,"%s",fileName);
        sprintf(tempStr,"%s%s",tempStr,time);
        createMd5(tempStr,newFilename);
        int filesize = ntohl(*(uint32_t*)buf);
        *buf += sizeof(uint32_t);
        char fullpath[500]={0};
        createfullpath(tempPath,newFilename,fullpath);
        FILE * flogx=fopen(fullpath,"a");
    	if (NULL==flogx){
	    	free(contentOrFileName);
	    	return -1;
	    }    		
	    residueLen = residueLen -(sizeof(uint16_t)+sizeof(uint32_t)+length);
        fwrite(buf,recvlens-residueLen,1,flogx);
		while(header->total>recvlens){			
			int ret = -1;
			if((header->total-recvlens)>MAXBUF){							
				ret = recv(sockfd,recvBuf,MAXBUF,0);				
			}else{							
				ret = recv(sockfd,recvBuf,header->total-recvlens,0);
			}
			if(ret<=0){
				free(contentOrFileName);
				fclose(flogx);
				return -1;
			}
			fwrite(recvBuf,ret,1,flogx);
			recvlens += ret;
		}
		fclose(flogx);
	}
	//if(contentOrFileName)
	//	free(contentOrFileName);	
	return 0;
}
/********************
	it is do not have file content in return buffer
*********************/
int createServerMessage(int sock,int serverid,push_message_info_t* info,char* tmpPath){
	unsigned char* retbuf = NULL;
	int total = 0;
	server_header_2_t * header = createServerHeader(serverid,COMMAND_MESSAGE,info->messagetype);
	//count length
	total = sizeof(server_header_2_t)+sizeof(uint32_t)+sizeof(uint16_t) + 
			strlen(info->to)+sizeof(uint16_t)+strlen(info->messageid);	

	if(info->messagetype == MESSAGE_TYPE_TXT){
		total += sizeof(uint16_t)+strlen(info->content);
	}else{
		total += sizeof(uint16_t)+strlen(info->orgFileName);
		total += sizeof(uint32_t);
		char fullpath[500]={0};
		createfullpath(tmpPath,info->content,fullpath);
		total += get_file_size(fullpath);
	}
	header->total = total;	
	printf("server message total:%d",total);
	//total+=20;
	retbuf = malloc(total);
	memset(retbuf,0,total);	
	void* buf = retbuf;
	memcpy(buf,header,sizeof(server_header_2_t));	
	dump_data(retbuf,total);
	buf += sizeof(server_header_2_t);	
	//send time
	*(uint32_t*)buf = htonl((uint32_t)info->sendtime_l);
	buf += sizeof(uint32_t);
	//to
	*(uint16_t*)buf = htons(strlen(info->to));
	buf += sizeof(uint16_t);
	memcpy(buf,info->to,strlen(info->to));
	buf += strlen(info->to);
	//messageid
	*(uint16_t*)buf = htons(strlen(info->messageid));
	buf += sizeof(uint16_t);
	memcpy(buf,info->messageid,strlen(info->messageid));
	buf += strlen(info->messageid);
	//message content
	//buf = createServerBuffforMessage(buf,serverid,info,tmpPath);
	
	if(info->messagetype == MESSAGE_TYPE_TXT){
		*(uint16_t*)buf = htons(strlen(info->content));
		buf += sizeof(uint16_t);
		memcpy(buf,info->content,strlen(info->content));
	}else{
		//TODO
		*(uint16_t*)buf = htons(strlen(info->orgFileName));
		buf += sizeof(uint16_t);
		memcpy(buf,info->orgFileName,strlen(info->orgFileName));
		buf += strlen(info->orgFileName);
		char fullpath[500]={0};
		createfullpath(tmpPath,info->content,fullpath);
		unsigned long filesize = get_file_size(fullpath);
		*(uint32_t*)buf = htonl(filesize);
		buf += sizeof(uint32_t);//TODO max send sizeof(uint32_t)
		int ret = 0;
		unsigned char* tempBuf = readtofile(tmpPath,info->content,&ret);		
		memcpy(buf,tempBuf,ret);
		free(tempBuf);
	}		
	free(header);	
	int sendlen = 0;
	int sumlen = 0;
	//TODO
	dump_data(retbuf,total);
	if(sock != -1){			
		do{
			if((total-sumlen) > MAXBUF)
				sendlen = send(sock,retbuf+sumlen,MAXBUF,0);	
			else
				sendlen = send(sock,retbuf+sumlen,total-sumlen,0);	
			sumlen += sendlen;	
		}while(sumlen<total);
	}
	printf("send len is:%d\n",sumlen);
	//TODO
	free(retbuf);
	return sumlen;
}
int createServerMessageReply(int sock,int serverid,char* messageid){	
	server_header_2_t* sheader = createServerHeader(serverid,COMMAND_YES,MESSAGE_TYPE_NULL);
 	int length = sizeof(server_header_2_t)+sizeof(uint16_t)+strlen(messageid);
 	sheader->total = length;
 	void* buff = malloc(length);
 	memcpy(buff,sheader,sizeof(server_header_2_t));
 	buff += sizeof(server_header_2_t);
 	*(uint16_t*)buff = htons(strlen(messageid));
 	buff += sizeof(uint16_t);
 	memcpy(buff,messageid,strlen(messageid));
 	int ret = send(sock,buff,length,0);
 	return ret;
}