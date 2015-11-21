#ifndef SOCKET_PLUSH_H
#define SOCKET_PLUSH_H


#define recv( sock,buff,len,flags) socket_recv(sock,buff,len)
#define send( sock,buff,len,flags) socket_send2(sock,buff,len)

int addepollevent(int fd,void* ptr);
int modepollevent(int fd,void* ptr);
void setnonblocking(int sockfd);
int mksock(int type,int port);
#endif