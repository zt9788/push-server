#ifndef CLIENT_H
#define CLIENT_H


#include "extended_storage.h"
#include "list.h"

typedef struct send_message
{
    int len;
//    unsigned char* buff;
    char messageid[64];
    char* message;
    unsigned char messagetype;
    int delytime;    
    list_t* sendto;
} send_message_t;

int createConnect(char* serverip,int port);
void sendhelo(int sockfd,char* drivceId);
void sendping(int sockfd);
//int message_processer(char* serverip,int port,char* drivceId,char* tempPath);
int start_client(char* servreip,int port,char* drivceId,char* tempPath,void* ptr);
void stop_client();

void sendMessage(char* clientMessageid,int delytime,
							char* message,
							int messagetype,
							list_t* sendtoList);							
//------------this is block function-------------

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


#endif