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



#include <malloc.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include "list.h"
#include "util.h"
#include "sc.h"
#include "parse_command.h"
#include "extended_protocol.h"
#include "cJSON.h"



//header byte[2]   messages
//        lenth    username

char* parseServerUserReg(int sock,void* buf,char* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));	
	//free(str);
	str = cJSON_GetObjectItem(json,"userid")->valuestring;	
	int isSucess = cJSON_GetObjectItem(json,"result")->valueint;	
	//char* userid = malloc(strlen(str)+1);
	if(isSucess)
		strncpy(outResult,str,strlen(str));			
	free(txt);
	cJSON_Delete(json);
	return outResult;
}
int createServerUserReg(int sock,int serverid,int isSuccess,char* userid,char* username){
	server_header_2_t* header = createServerHeader(serverid,COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_REG);
	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json,"result",isSuccess);
	cJSON_AddStringToObject(json,"username",username);	
	cJSON_AddStringToObject(json,"userid","userid");
	char* str = cJSON_Print(json);	
	int total = sizeof(server_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(server_header_2_t));
	buf += sizeof(server_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	int ret =send(sock,bufs,total,0);
	free(bufs);
	cJSON_Delete(json);
	//free(str);
	return ret;	
}
int createClientUserReg(int sock,int clienttype,char* username){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_REG,clienttype);	
	cJSON* json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"username",username);	
	char* str = cJSON_Print(json);	
	printf("%s,%d",str,strlen(str));
	int total = sizeof(client_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(client_header_2_t));
	
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	//dump_data(str,strlen(str));
	dump_data(bufs,total);
	int ret =send(sock,bufs,total,0);
	printf("%d\n",ret);
	free(bufs);
	
//	free(str);
	cJSON_Delete(json);	
	return ret;	
}
char* parseClientUserReg(int sock,void* buf,int serverid,char* drivceId,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));	
	char userid[64]={
		0
	};
#ifndef CLIENTMAKE	
	char* uid = regUser(username,drivceId,userid);
	int isSucess = 0;
	if(uid != NULL)
		isSucess = 1;
	*outResult = createServerUserReg(sock,serverid,isSucess,userid,username);
#endif	
	free(txt);
	cJSON_Delete(json);
	return username;
}


int createClientUserLogin(int sock,int clienttype,char* username){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_LOGIN,clienttype);	
	cJSON* json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"username",username);	
	char* str = cJSON_Print(json);	
	printf("%s,%d",str,strlen(str));
	int total = sizeof(client_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(client_header_2_t));
	
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	//dump_data(str,strlen(str));
	dump_data(bufs,total);
	int ret =send(sock,bufs,total,0);
	printf("%d\n",ret);
	free(bufs);
	
//	free(str);
	cJSON_Delete(json);	
	return ret;	
}

char* parseClientUserLogin(int sock,void* buf,int serverid,char* drivceId,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));		
#ifndef CLIENTMAKE	
	user_info_t* clientinfo=NULL;
	user_info_t* uinfo = userLogin(username,drivceId,&clientinfo);//regUser(username,userid);
	int isSucess = 0;
	if(uinfo != NULL)
		isSucess = 1;
	*outResult = createServerUserLogin(sock,serverid,isSucess,username);
	//freeClientInfo(clientinfo);
	freeUserInfo(clientinfo);
#endif	
	free(txt);
	cJSON_Delete(json);
	return username;
}

int createServerUserLogin(int sock,int serverid,int isSuccess,char* username){
	server_header_2_t* header = createServerHeader(serverid,COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_LOGIN);
	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json,"result",isSuccess);
	cJSON_AddStringToObject(json,"username",username);	
	//cJSON_AddStringToObject(json,"userid","userid");
	char* str = cJSON_Print(json);	
	int total = sizeof(server_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(server_header_2_t));
	buf += sizeof(server_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	int ret =send(sock,bufs,total,0);
	free(bufs);
	cJSON_Delete(json);
	//free(str);
	return ret;	
}

int parseServerUserLogin(int sock,void* buf,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));	
	//free(str);
	//str = cJSON_GetObjectItem(json,"userid")->valuestring;	
	int isSucess = cJSON_GetObjectItem(json,"result")->valueint;	
	//char* userid = malloc(strlen(str)+1);
	*outResult = isSucess;	
	free(txt);
	cJSON_Delete(json);
	return isSucess;
}



int createClientUserFindUser(int sock,int clienttype,char* username){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_FIND_USERNAME,clienttype);	
	cJSON* json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"username",username);	
	char* str = cJSON_Print(json);	
	char* str2 = cJSON_GetObjectItem(json,"username")->valuestring;
	printf("\n");
	printf("str=%s,str2=%s\n",str,str2);
	printf("str=%s,%d\n",str,strlen(str));
	dump_data(str2,strlen(str2));
	int total = sizeof(client_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(client_header_2_t));
	
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	//dump_data(str,strlen(str));
	dump_data(bufs,total);
	int ret =send(sock,bufs,total,0);
	printf("%d\n",ret);
	free(bufs);	
//	free(str);
	cJSON_Delete(json);	
	return ret;	
}


char* parseClientUserFindUser(int sock,void* buf,int serverid,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	printf("json=%s\n",txt);
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));
	printf("username = %s\n",username);	

#ifndef CLIENTMAKE	
	user_info_t* userinfo=NULL;
	user_info_t* uinfo = getUserInfoByName(username,&userinfo);//userLogin(username,drivceId,&clientinfo);//regUser(username,userid);
	int isSucess = 0;
	if(uinfo != NULL)
		isSucess = 1;
	*outResult = createServerUserFindUser(sock,serverid,userinfo);
	freeUserInfo(userinfo);
#endif	
	free(txt);
	cJSON_Delete(json);
	return username;
}
int createServerUserFindUser(int sock,int clienttype,user_info_t* userinfo){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_LOGIN,clienttype);	
	cJSON* json = cJSON_CreateObject();
	if(userinfo != NULL){
		cJSON_AddStringToObject(json,"username",userinfo->username);	
		cJSON_AddStringToObject(json,"userid",userinfo->userid);
		cJSON_AddNumberToObject(json,"isSuccess",1);
		cJSON* arr = cJSON_CreateArray();
		if(userinfo->drivces){
			list_node_t* node = NULL;
			int i = 0;
			for(i=0;i<userinfo->drivces->len;i++)
			{
				node = list_at(userinfo->drivces,i);
				printf("pop list=%s\n",node->val);
				cJSON_AddItemToArray(arr,cJSON_CreateString(node->val));	
			}															
		}
		cJSON_AddItemToObject(json,"driveces",arr);
	}
	else{
		cJSON_AddNumberToObject(json,"isSuccess",0);
	}	
	char* str = cJSON_Print(json);	
	printf("%s,%d",str,strlen(str));
	int total = sizeof(server_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(server_header_2_t));
	buf += sizeof(server_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	int ret =send(sock,bufs,total,0);
	free(bufs);
	cJSON_Delete(json);
	//free(str);
	return ret;	
}

int parseServerUserFindUser(int sock,void* buf,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}
	int isSucess = cJSON_GetObjectItem(json,"result")->valueint;
	char* username = NULL;
	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	if(str != NULL){
		username = malloc(strlen(str)+1);
		strncpy(username,str,strlen(str));	
	}	
	//free(str);
	//str = cJSON_GetObjectItem(json,"userid")->valuestring;	
		
	//char* userid = malloc(strlen(str)+1);
	*outResult = isSucess;	
	free(txt);
	cJSON_Delete(json);
	return *outResult;
}


int createClientUserAddUser(int sock,int clienttype,char* username,char* friendname){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_ADD_FRIEND,clienttype);	
	cJSON* json = cJSON_CreateObject();
	cJSON_AddStringToObject(json,"username",username);	
	cJSON_AddStringToObject(json,"friendname",friendname);	
	char* str = cJSON_Print(json);	
	int total = sizeof(client_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(client_header_2_t));	
	buf += sizeof(client_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));	
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	dump_data(bufs,total);
	int ret =send(sock,bufs,total,0);
	printf("%d\n",ret);
	free(bufs);	
//	free(str);
	cJSON_Delete(json);	
	return ret;	
}



char* parseClientUserAddUser(int sock,void* buf,int serverid,int* outResult){
	uint16_t length = ntohs(*(uint16_t*)buf);
	buf += sizeof(uint16_t);
	char *txt = malloc(length);
	memcpy(txt,buf,length);
	buf += length;	
	cJSON* json = cJSON_Parse(txt);
	if(!json){
		free(txt);
		return NULL;
	}

	char* str = cJSON_GetObjectItem(json,"username")->valuestring;	
	char* username = malloc(strlen(str)+1);
	strncpy(username,str,strlen(str));
	str = cJSON_GetObjectItem(json,"username")->valuestring;
	char* friendname = 	malloc(strlen(str)+1);
	strncpy(friendname,str,strlen(str));
#ifndef CLIENTMAKE	
	user_info_t* userinfo=NULL;
	user_info_t* uinfo = addFriendsUseName(username,friendname);//getUserInfoByName(username,&userinfo);//userLogin(username,drivceId,&clientinfo);//regUser(username,userid);
	
	int isSucess = 0;
	if(uinfo != NULL)
		isSucess = 1;
	*outResult = createServerUserAddUser(sock,serverid,userinfo);
	freeUserInfo(userinfo);
#endif	
	free(txt);
	cJSON_Delete(json);
	return username;
}


int createServerUserAddUser(int sock,int clienttype,user_info_t* userinfo){
	client_header_2_t* header = createClientHeader(COMMAND_OTHER_MESSAGE,MESSAGE_TYPE_USER_ADD_FRIEND,clienttype);	
	cJSON* json = cJSON_CreateObject();
	if(userinfo != NULL){
		cJSON_AddStringToObject(json,"username",userinfo->username);	
		cJSON_AddStringToObject(json,"userid",userinfo->userid);
		cJSON_AddNumberToObject(json,"isSuccess",1);
		cJSON* arr = cJSON_CreateArray();
		if(userinfo->drivces){
			list_node_t* node = NULL;
			int i = 0;
			for(i=0;i<userinfo->drivces->len;i++)
			{
				node = list_at(userinfo->drivces,i);
				cJSON_AddItemToArray(arr,cJSON_CreateString(node->val));	
			}															
		}
		cJSON_AddItemToObject(json,"driveces",arr);
	}
	else{
		cJSON_AddNumberToObject(json,"isSuccess",0);
	}	
	char* str = cJSON_Print(json);	
	printf("%s,%d",str,strlen(str));
	int total = sizeof(server_header_2_t)+sizeof(uint16_t)+strlen(str);
	void* bufs = malloc(total);
	void* buf = bufs;
	header->total = total;
	memcpy(buf,header,sizeof(server_header_2_t));
	buf += sizeof(server_header_2_t);
	*(uint16_t*)buf = htons(strlen(str));
	buf += sizeof(uint16_t);
	memcpy(buf,str,strlen(str));
	int ret =send(sock,bufs,total,0);
	free(bufs);
	cJSON_Delete(json);
	//free(str);
	return ret;	
}