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
#ifndef PARSE_COMMADN_H
#define PARSE_COMMADN_H


#include <malloc.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include "list.h"
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
						char* contentOrFileName,list_t* sendto);
int parseClientMessage(int sockfd,void* bufs,client_header_2_t* header,char* driveceId,
		char* token,int recvlen,char* fromip,char* tempPath,char* outMessageid);
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
int createServerMessage(int sock,int serverid,push_message_info_t* info,char* tmpPath);

int parseServerMessage(int sockfd,void* bufs,server_header_2_t* header,
		char* fromdriveceId,char* messageid,char** content,int recvlen,char* tempPath);
		
int createServerMessageReply(int sock,int serverid,char* messageid);		
#endif /* !PARSE_COMMADN_H */