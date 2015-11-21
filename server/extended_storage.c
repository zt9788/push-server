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
#include "extended_storage.h"
#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif

user_info_t* addFriendsUseName(char* username,char* friendusername){
	char userid[64]={0};
	char userid2[64]={0};
	getUserIdByUsername(username,userid);
	getUserIdByUsername(friendusername,userid2);
	int redisId=0;
	redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = redisCommand(redis,"sadd %s_friends %s",userid,userid2);
		freeReplyObject(reply);	
	returnRedis(redisId);
	user_info_t* userinfo = NULL;
	getUserInfoByUserId(friendusername,&userinfo);
	return userinfo;
	
}
user_info_t* getUserInfoByUserId(char* userid,user_info_t** userinfo){
	char username[64]={0};
	//getUserIdByUsername(usenrame,userid);
	list_t * list = NULL;
	getUsernameByUserId(userid,username);
	user_info_t *info = *userinfo;
	if (info == NULL){
		info = malloc(sizeof(user_info_t));	
	}	
	memset(*userinfo,0,sizeof(user_info_t));
	strncpy(info->userid,userid,strlen(userid));
	strncpy(info->username,username,strlen(username));
	
	getUserDrivceIdByUserId(userid,&list);
	if(list)
		info->drivces = list;
	*userinfo = info;
	return info;
}
user_info_t	* getUserInfoByName(char* username,user_info_t** userinfo){
	char userid[64]={0};
	getUserIdByUsername(username,userid);
	user_info_t* info = *userinfo;
	if (info == NULL){
		info = malloc(sizeof(user_info_t));	
	}
	memset(info,0,sizeof(user_info_t));
	strncpy(info->userid,userid,strlen(userid));
	strncpy(info->username,username,strlen(username));
	list_t * list = NULL;
	getUserDrivceIdByUserId(userid,&list);
	if(list)
		info->drivces = list;
	*userinfo = info;
	return info;
}
list_t* getUserDrivceIdByUserId(char* userid,list_t** outList){	
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	list_t* list = *outList;
	if(list == NULL)
		list = list_new();
	reply = redisCommand(redis,"SMEMBERS %s_drivce_list",userid);
	int i =0 ;
	if(reply != NULL){
		for(i=0;i<reply->elements;i++){
			list_lpush(list,list_node_new(reply->element[i]->str));
		}
	}else{
		list_destroy(list);
	}	
	freeReplyObject(reply);	
	returnRedis(redisId);
	*outList = list;
	return list;
}
void freeUserFriends(list_t* list){
	if(list->len>0){
		list_node_t* ln = list_lpop(list);
		if(ln != NULL){			
			user_info_t* userinfo = ln->val;
			freeUserInfo(userinfo);
			free(ln);
		}
	}
}
list_t* getUserFriends(char* userid,list_t** outList){
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	list_t* list = *outList;
	if(list == NULL)
		list = list_new();
	//TODO 
	list->free=freeClientInfo;
	reply = redisCommand(redis,"SMEMBERS %s_friends",userid);
	int i =0 ;
	if(reply != NULL){
		for(i=0;i<reply->elements;i++){
			user_info_t* userinfo = NULL;
			getUserInfoByUserId(reply->element[i]->str,&userinfo);
			if(userinfo != NULL)
				list_lpush(list,list_node_new(userinfo));	
		}		
	}else{
		list_destroy(list);
	}	
	freeReplyObject(reply);	
	returnRedis(redisId);
	*outList = list;
	return list;
}

char* getUserIdByUsername(char* username,char* outUserid){
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = redisCommand(redis,"get %s",username);
	if(reply != NULL)
		strncpy(outUserid,reply->str,reply->len);
	freeReplyObject(reply);	
	returnRedis(redisId);
	return outUserid;
}
char* getUsernameByUserId(char* userid,char* outUsername){
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = redisCommand(redis,"get %s",userid);
	if(reply != NULL)
		strncpy(outUsername,reply->str,reply->len);
	freeReplyObject(reply);	
	returnRedis(redisId);
	return outUsername;
}
/*
char* getUserNameByDrivce(char* drivceId){
	return NULL;
}
*/
char* regUser(char* username,char* outUserid){
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
	reply = redisCommand(redis,"get %s",username);
	char userid[64]={0};
	if(reply != NULL){
		strncpy(userid,reply->str,reply->len);
	}
	freeReplyObject(reply);	
	if(strlen(userid) > 0){
		returnRedis(redisId);
		return NULL;
	}			
	createGUID(userid);
	redisAppendCommand(redis,"set %s %s",userid,username);//1
	redisAppendCommand(redis,"set %s %s",username,userid);//2
	redisAppendCommand(redis,"sadd reg_user_list %s",userid);//3	
	int i =0;
	for(i=0;i<3;i++){
		redisGetReply(redis,(void**)&reply);
        freeReplyObject(reply);//1	
	}
	returnRedis(redisId);
	strncpy(outUserid,userid,strlen(userid));
	return outUserid;
}
user_info_t* isUserLogin(char* userid){
	/*
	if(userinfo == NULL) return NULL;
	if(!userinfo->drivces) return NULL;
	if(userinfo->drivces->len == 0) return NULL;
	*/
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;
 	user_info_t* userinfo = NULL;
	reply = redisCommand(redis,"SISMEMBER login_user_list %s",userid);//1
	if(reply->integer == 1){		
		getUserInfoByUserId(userid,&userinfo);
		//returnRedis(redisId);	
		//return userinfo;
	}
	freeReplyObject(reply);	
	returnRedis(redisId);	
	
	return userinfo;
} 
void freeUserInfo(user_info_t* userinfo){
	if(userinfo){
		if(userinfo->drivces){
			list_destroy(userinfo->drivces);
		}
		free(userinfo);
	}
}
user_info_t* userLogin(char* username,char* drivceId,user_info_t** outUserinfo){
	int redisId=0;
	int ret = 0;
    redisContext* redis = getRedis(&redisId);
	redisReply* reply = NULL;	
	char userid[64]={0};
	getUserIdByUsername(username,userid);
	redisAppendCommand(redis,"sadd login_user_list %s",userid);//1
	redisAppendCommand(redis,"sadd %s_drivce_list %s",userid,drivceId);//2
	freeReplyObject(reply);	
	returnRedis(redisId);
	freeReplyObject(reply);	
	returnRedis(redisId);	
	user_info_t* userinfo = *outUserinfo;
	if (userinfo == NULL){
		userinfo = malloc(sizeof(user_info_t));	
	}
	memset(userinfo,0,sizeof(user_info_t));
	strncpy(userinfo->userid,userid,strlen(userid));
	strncpy(userinfo->username,username,strlen(username));
	*outUserinfo = userinfo;
	return userinfo;
}
void userLogout(char* userid){
	int redisId=0;
	redisReply* reply = NULL;
    redisContext* redis = getRedis(&redisId);    
    redisAppendCommand(redis,"srem %s_drivce_list %s",userid);//1
    redisAppendCommand(redis,"srem login_user_list %s",userid);//2
    redisGetReply(redis,(void**)&reply);
    freeReplyObject(reply);	
    redisGetReply(redis,(void**)&reply);
    freeReplyObject(reply);	
	returnRedis(redisId);
}

