#ifndef SOCKET_PLUSH_H
#define SOCKET_PLUSH_H

#ifdef __EPOLL__
#include <sys/epoll.h>
#endif
#define recv( sock,buff,len,flags) socket_recv(sock,buff,len)
#define send( sock,buff,len,flags) socket_send2(sock,buff,len)

#define MAX_CONNECTION_LENGTH 102400
#define MAX_EVENTS 102400  
#define REVLEN 1024

int addepollevent(int fd,void* ptr,int epollfd);
int modepollevent(int fd,void* ptr,int epollfd);
void setnonblocking(int sockfd);
int mksock(int type,int port);
#endif