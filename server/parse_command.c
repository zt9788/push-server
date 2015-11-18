
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
						char* contentOrFileName,short len,...){
	client_header_2_t* header = createClientHeader(COMMAND_MESSAGE,messsagetype,clienttype);
	char* s = NULL;
	int i = 0;
	int length = sizeof(client_header_2_t);	
	char* tempBufhead = malloc(MAX_DRIVCEID_LENGTH*len+sizeof(uint16_t)*(len+1));
	int templenth = 0;
	void* tempBuf = tempBufhead;
	va_list argp;
	va_start(argp,len);
	
	*(uint16_t*)tempBuf = htons(len);
	tempBuf += sizeof(uint16_t);
	templenth += sizeof(uint16_t);
	for(i=0;i<len;i++){
		s = va_arg(argp,char*) ;
		if(s==NULL || strlen(s) <=0){
			continue;
		}
		*(uint16_t*)tempBuf=htons(strlen(s));
		tempBuf += sizeof(uint16_t);
		memcpy(tempBuf,s,strlen(s));
		tempBuf += strlen(s);
		
		templenth += sizeof(uint16_t);
		templenth += strlen(s);
	}
	va_end(argp);			
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
	//TODO
	dump_data(retbuf,length);
	sendlen = send(sock,retbuf,length,0);
	free(retbuf);
	return sendlen;
}
int parseClientMessage(int sockfd,void* bufs,client_header_2_t* header,char* driveceId,
		char* token,int recvlen,char* fromip,char* tempPath){
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
			sendto[i] = malloc(len);
			buf += sizeof(uint16_t);
			memcpy(sendto[i],buf,len);
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
		contentOrFileName = malloc(length);
		int templen = -1;
		if(recvlen >= header->total-sizeof(client_header_2_t)){
			memcpy(contentOrFileName,buf,length);		
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
	for(i=0;i<sendtocount;i++){
		push_message_info_t* info = NULL;
#ifndef CLIENTMAKE	
		putmessageinfo2(contentOrFileName,sendto[i],contentOrFileName,
					newFilename,delytime,driveceId,
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

void* parseServerHeader(void* bufs,server_header_2_t	* header){
	if(bufs == NULL)
		return NULL;
	char* buf = bufs;
	if(strlen(buf) < sizeof(server_header_2_t))
		return NULL;
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
	struct server_header_2 * header = malloc(sizeof(struct server_header_2));
	header->command == command;
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
void* parseServerBufferMessage(){
	
}
/********************
	it is do not have file content in return buffer
*********************/
void* createServerMessagereply(int serverid,push_message_info_t* info,char* tmpPath){
	unsigned char* retbuf = NULL;
	int total = 0;
	struct server_header_2 * header = createServerHeader(serverid,COMMAND_MESSAGE,info->messagetype);
	total = sizeof(struct server_header_2)+2+strlen(info->to)+2+strlen(info->messageid);	
	if(info->messagetype == MESSAGE_TYPE_TXT){
		total += 2+strlen(info->content);
	}else{
		total += 2+strlen(info->orgFileName);
		total += sizeof(uint32_t);
		char fullpath[500]={0};
		createfullpath(tmpPath,info->content,fullpath);
		total += get_file_size(fullpath);		
	}
	header->total = total;
	
	retbuf = malloc(total);
	unsigned char* buf = retbuf;
	memcpy(buf,header,sizeof(struct server_header_2));
	buf += sizeof(struct server_header_2);
	//memcpy(buf,htons(strlen(info->to)),sizeof(uint16_t));
	*(uint16_t*)buf = htons(strlen(info->to));
	buf += 2;
	memcpy(buf,info->to,strlen(info->to));
	buf += strlen(info->to);
	//memcpy(buf,htons(strlen(info->messageid)),sizeof(uint16_t));
	*(uint16_t*)buf = htons(strlen(info->messageid));
	buf += 2;
	memcpy(buf,info->messageid,sizeof(info->messageid));
	buf += strlen(info->messageid);
	buf = createServerBuffforMessage(buf,serverid,info,tmpPath);
	free(header);
	return retbuf;
}
