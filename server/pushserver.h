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

#ifndef PUSHSERVER_H
#define PUSHSERVER_H


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <hiredis/hiredis.h>
#include <poll.h>
#include <netinet/in.h>

#include <semaphore.h>
#include <fcntl.h>
#ifdef __EPOLL__
#include <sys/epoll.h>
#endif
#include "sc.h"
#include "redis_pool.h"
#include "parse_command.h"

#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif 
#include "messagestorage.h"
#define TRUE 1
#define FALSE 0
#ifndef BYTE
//typedef unsigned char BYTE;
#endif



typedef struct thread_struct{
    int sockfd;
    pthread_mutex_t lock;
    int isexit;
    char token[255];
}thread_struct;

typedef struct pushstruct
{
	CLIENT* client;
	pthread_mutex_t lock;
	int isexit;
	int timeout;
	//struct sockaddr_in addr;
} pushstruct;


void send_OK(int sockfd,int type);
int new_connection(int slisten,//struct sockaddr_in* addr,
			CLIENT* client,int* maxfd,int epollfd);
int recvMessage(int sockfd,CLIENT_HEADER *header,void* buffer,CLIENT* client);
void close_socket(CLIENT* client);
void send_helo(int sockfd,CLIENT_HEADER	*header,CLIENT* client,char* token);
void* send_helo_to_client_message(CLIENT* client);
//int send_pushlist_message(push_message_info_t* messageinfo,CLIENT* client);
void* sendMessage(void* sendinfo);/*send_info_t*/

#endif /* !PUSHSERVER_H */