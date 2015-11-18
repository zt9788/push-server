#ifndef PARSE_COMMADN_H
#define PARSE_COMMADN_H


#include <malloc.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include "util.h"
#include "sc.h"
#include "messagestorage.h"
/*
typedef struct send_user_list{
	char* drivceId;
	struct send_user_list* next;
}send_user_list_t;
*/

void* createServerHelo(int serverid,
						unsigned char isOpenMessageResponse,
						unsigned char isOpenFileSocket,
						unsigned	char isOpenPingResponse);
						
void* createClientPing(unsigned	char clienttype	);
void* createServerPing(int serverid	);
int  createClientHelo(void** retbuffer,unsigned	char clienttype,char* drivceId,char* iostoken);
void* parseServerHeader(void* bufs,server_header_2_t	* header);
void* parseClientHeader(void* bufs,client_header_2_t* header);
int parseClientHelo(void* buf,char* drivceId,char* iostoken,int clienttype);
void* parseServerHelo(void* buf,server_config_to_client_t* config);
int createClientMessage(int sock,unsigned	char messsagetype,unsigned	char clienttype,
						short delytime,
						char* contentOrFileName,short len,...);
int parseClientMessage(int sockfd,void* bufs,client_header_2_t* header,char* driveceId,
		char* token,int recvlen,char* fromip,char* tempPath);
client_header_2_t* createClientHeader(unsigned char command,unsigned char messagetype,unsigned char clienttype);
server_header_2_t* createServerHeader(int serverid,unsigned char command,unsigned char messagetype);
server_config_to_client_t* createServer2ClientConfig(server_config_to_client_t* config,
													unsigned 	char isOpenMessageResponse,
													unsigned	char isOpenFileSocket,
													unsigned	char isOpenPingResponse);
void* createServerBuffforMessage(void* bufs,
					int serverid,
					push_message_info_t* info,
					char* tmpPath);
void* createServerMessagereply(int serverid,push_message_info_t* info,char* tmpPath);


#endif /* !PARSE_COMMADN_H */