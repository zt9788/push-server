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

#ifndef MESSAGE_STROE_H
#define MESSAGE_STROE_H

//#include "pushserver.h"

typedef struct push_message_info{	
	int retrycount;
	int state;
	int messagetype;	
	char messageid[64];
	char submessageid[64];
	char to[64];/*to drivceId*/
	char orgFileName[255];		
	char sendtime[50];
	long sendtime_l;
	char form[64];
	char fromip[20];	
	char* content;	
	int timeout;
	int error;/*0 is OK not 0 is has error*/
	char* errStr;
}push_message_info_t;

typedef struct client_info{
	char drivceId[64];
	int clienttype;
	int clinetport;
	int clientid;
	int serverid;
	char clientip[20];
	char serverip[20];
	char logintime[50];	
	int error;/*0 is OK not 0 is has error*/
	char* errStr;
}client_info_t;

typedef struct send_info{
	CLIENT* client;
	push_message_info_t* info;	
}send_info_t;

typedef struct user_friends{
	int isOnline;
	char userid[64];
	char drivceId[64];
	struct user_friends* next;
}user_friends_t;



void print_push_info(push_message_info_t* info);
void print_client_info(client_info_t* info);

client_info_t* putclientinfo(struct CLIENT* client,
					int serverid,
					char* serverip,
					char* drivceId,
					int clienttype,
					client_info_t** info);
					
client_info_t* newClient(struct CLIENT* client,
 					int serverid,
					char* serverip,
					char* drivceId,
					int clienttype,
					client_info_t** info);
										
client_info_t* getClientInfo(client_info_t** info,char* drivceId);

push_message_info_t* putmessageinfo(char* message,
									char* tochar,
									char* fileName,
									char* newFilename,
									int timeout,
									CLIENT_HEADER* header,
									struct CLIENT* client,
									int priority,
									push_message_info_t** info);
push_message_info_t* putmessageinfo2(char* messageid,
									char* message,
									char* tochar,
									char* fileName,
									char* newFilename,
									int timeout,
									char* from,
									char* fromip,
									int messagetype,
									int priority,
									push_message_info_t** info_in);									
push_message_info_t* getmessageinfo(char* pushid,push_message_info_t** info);
void freePushMessage(push_message_info_t* messageinfo);
void freeClientInfo(client_info_t* clientinfo);
int isClientOnline(char* drivceId);
int isHasMessageInfo(char* messageid);		
void removeClient(struct CLIENT* client);	
int getNextNoReadMessageId(char* drivceId,char* messageid);
int getNoReadMessageSize(char* drivceId);
int getNextMessageId(int serverid,char* messageid);		
void saveInNoread(char* drivceId,char* messageid);
void messageIsSendOK(push_message_info_t* messageinfo,client_info_t	* clientinfo);
void messageIsSendOK2(push_message_info_t* messageinfo,int toClientServerid);
int getDelyMessage(char* messageid);	
int getClientDrivceIdFromMessage(char* messageid,char* drivceId);
void saveInPushlist(char* messageid,int serverid);			
void changeClientId(char* drivceid,int clientid);
					
#endif /* !MESSAGE_STROE_H */