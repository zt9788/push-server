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
#ifndef CLIENT_H
#define CLIENT_H


#include "extended_storage.h"
#include "list.h"


#define CLIENT_TYPE 1

typedef struct client_config{
	char serverip[64];
	int port;
	char drivceId[64];
	char tempPath[500];
	void* ptr;
}client_config_t;

typedef struct send_message
{
    int len;
    char messageid[64];
    char* message;
    unsigned char messagetype;
    int delytime;    
    list_t* sendto;
} send_message_t;


//inner function
void* send_message_processer(void* data);
int message_processer(char* serverip,int port,char* drivceId,char* tempPath,void* ptr);
int createConnect(char* serverip,int port);
void* ping_process(void* data);
void sendhelo(int sockfd,char* drivceId);
void sendping(int sockfd);

//public function
int start_client(char* servreip,int port,char* drivceId,char* tempPath,void* ptr);
void stop_client();

void sendMessage(char* clientMessageid,int delytime,
							char* message,
							int messagetype,
							list_t* sendtoList);							
//------------this is block function-------------
//----------and if you are android you need call it in jni functoin

user_info_t* user_register(char* userName,
			char* password/* the password is nothing to do*/);

user_info_t* user_login(char* name,
			char* password/* the password is nothing to do*/);					
			
user_info_t* add_Friend(char* friendName);

user_info_t* find_User(char* userName);

list_t*/*<user_info_t>*/ get_MyFriends(char* username);

void del_Friend(char* userName);

void say_Bye();
//----------------end----------------------------

//this is call back function ,in jni or ios need to implement it
void client_error(int errorCode,char* errs,void* data);
void recvData(char* serverMessageId,char* fromDrivceId,char* str,void* data);
void recvFile(char* serverMessageId,char* fromDrivceId,char* fileName,void* data);
void sendMessageResponse(char* clientMessageid,char* serverMessageid, int responseCode,
			char* error,void* data);

//this is need to implement or fix on android or ios 
//need and (*g_jvm)->AttachCurrentThread(g_jvm,&env, NULL); etc.
void* send_message_thread(void* args);
void* recv_message_thread(void* args);
void* ping_thread(void* args);

#endif