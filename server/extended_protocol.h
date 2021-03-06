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
#ifndef EXTENDED_COMMADN_H
#define EXTENDED_COMMADN_H


#include <malloc.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include "list.h"
#include "util.h"
#include "sc.h"
#include "extended_storage.h"

int createServerUserReg(int sock,int serverid,int isSuccess,char* userid,char* username);
int createClientUserReg(int sock,int clienttype,char* username);
user_info_t* parseServerUserReg(int sock,void* buf,int* outResult);
char* parseClientUserReg(int sock,void* buf,int serverid,char* drivceId,int* outResult);

user_info_t* parseServerUserLogin(int sock,void* buf,int* outResult);
int createServerUserLogin(int sock,int serverid,int isSuccess,char* username);
char* parseClientUserLogin(int sock,void* buf,int serverid,char* drivceId,int* outResult);
int createClientUserLogin(int sock,int clienttype,char* username);

int createClientUserFindUser(int sock,int clienttype,char* username);
char* parseClientUserFindUser(int sock,void* buf,int serverid,int* outResult);
int createServerUserFindUser(int sock,int clienttype,user_info_t* userinfo);
user_info_t* parseServerUserFindUser(int sock,void* buf,int* outResult);

char* parseClientUserAddUser(int sock,void* buf,int serverid,int* outResult);
int createClientUserAddUser(int sock,int clienttype,char* username,char* friendname);
int createServerUserAddUser(int sock,int clienttype,user_info_t* userinfo);
user_info_t* parseServerUserAddUser(int sock,void* buf,user_info_t** outResult);

int createClientUserGetFriends(int sock,int clienttype,char* username);
char* parseClientUserGetFriends(int sock,void* buf,int serverid,int* outResult);
int createServerUserGetFriends(int sock,int clienttype,list_t* userinfos);
list_t* parseServerUserGetUser(int sock,void* buf,list_t** outResult);
#endif /* !EXTENDED_COMMADN_H */