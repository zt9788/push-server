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
#ifndef EXTENDS_PROTOCOL_H
#define EXTENDS_PROTOCOL_H

#include "list.h"

typedef struct user_info{
	char userid[64];
	char username[64];
	list_t* drivces;
}user_info_t;
/*
typedef struct user_drivce{
	unsigned char drivecetype;
	char drivceid[64];
	char token[64];
}user_drivce_t;
*/

char* getUsernameByUserId(char* userid,char* outUsername);
char* getUserIdByUsername(char* username,char* outUserid);
void freeUserInfo(user_info_t* userinfo);
user_info_t* isUserLogin(char* userid);
void freeUserInfo(user_info_t* userinfo);
char* regUser(char* username,char* outUserid);
user_info_t* addFriendsUseName(char* username,char* friendusername);
user_info_t* getUserInfoByUserId(char* userid,user_info_t** userinfo);
user_info_t* getUserInfoByName(char* username,user_info_t** userinfo);
list_t* getUserDrivceIdByUserId(char* userid,list_t** outList);
void freeUserFriends(list_t* list);
list_t* getUserFriends(char* userid,list_t** outList);
user_info_t* userLogin(char* username,char* drivceId,user_info_t** outUserinfo);
void userLogout(char* userid);
/***************************

	reg_user_list  Set<userid>  userid1,userid2 .... in reg
	login_user_list set<userid>   
	
	userid string username			in login
	username string userid			in reg,login
	
	%s(userid)_drivce_list Set<driveceid>  ==== find token from clientinfo		in login
	
	userid_friends Set<userid> userid


***************************


/*
typedef struct user_friend{
	userinfo_t* info;
	struct user_friend* next;
}user_friend_t;
*/

#endif