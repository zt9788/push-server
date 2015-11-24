#ifndef CLIENT_H
#define CLIENT_H


#include "extended_storage.h"
#include "list.h"


int createConnect(char* serverip,int port);
void sendhelo(int sockfd,char* drivceId);
void sendping(int sockfd);
int message_processer(char* serverip,int port,char* drivceId);
void start_client(char* servreip,char* port,char* drivceId);
void stop_client();

//------------this is block function-------------
void sendMessage(char* clientMessageid,
							char* message,
							int messagetype,
							list_t* sendtoList);

user_info_t* user_register(char* userName,
			char* password/* the password is nothing to do*/);

user_info_t* user_login(String name,
			String password/* the password is nothing to do*/);					
			
user_info_t* add_Friend(char* friendName);

user_info_t* find_User(char* userName);

list_t*/*<user_info_t>*/ get_MyFriends(char* username);

void del_Friend(char* userName);

void say_Bye();
//----------------end----------------------------

//this is call back function ,in jni or ios need to implement it
void client_error(int errorCode,char* errs);
void recvData(char* fromDrivceId,char* str);
void recvFile(char* fromDrivceId,char* fileName);
void sendMessageResponse(char* clientMessageid,char* serverMessageid, int responseCode,
			char* error);


#endif