
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

#undef send
#undef recv

int addepollevent(int fd,void* ptr)
{
#ifdef __EPOLL__
    struct epoll_event event;
    if(ptr == NULL)
        event.data.fd = fd;
    else
        event.data.ptr = ptr;
    event.events =  EPOLLIN|EPOLLET;  //EPOLLET
    if(epoll_ctl(g_epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        Log("epoll add fail : fd = %d\n", fd);
        return -1;
    }
#endif
    return 0;
}
int modepollevent(int fd,void* ptr)
{
#ifdef __EPOLL__
    struct epoll_event event;
    if(ptr == NULL)
        event.data.fd = fd;
    else
        event.data.ptr = ptr;
    if(event.events&EPOLOUT)
    	event.events =  event.events|EPOLLIN;//EPOLLIN|EPOLLET;  //EPOLLET
   	else
   		event.events =  event.events|EPOLLIN;//EPOLLIN|EPOLLET;  //EPOLLET
   		
    if(epoll_ctl(g_epollfd, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        Log("epoll add fail : fd = %d\n", fd);
        return -1;
    }
#endif
    return 0;
}

//设置socket连接为非阻塞模式
void setnonblocking(int sockfd) {
    int opts;

    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0) {
        perror("fcntl(F_GETFL)\n");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)\n");
        exit(1);
    }
}
int mksock(int type,int port)
{
    int sock;
    struct sockaddr_in  my_addr;
    if((sock = socket(AF_INET,type,0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    //bzero(&my_addr,sizeof(my_addr));
    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //close socket just now
    unsigned int value = 0x1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void *)&value,sizeof(value));
    return sock;

}
int socket_recv(int sock,void* buf,size_t len,int flag){
	int n = 0;
	int nread = 0;
	while ((nread = read(sock, buf + n, BUFSIZ-1)) > 0) {
	    n += nread;
	}
	if (nread == -1 && errno != EAGAIN) {
	    perror("read error");
	    return -1;
	}
	if(errno == EAGAIN){
		printf("this test code,need to delete errno == EAGAIN recv:%d\n",nread);
   	}
	if(nread == 0)
		return -1;
	return n;
}
int socket_send2(int sock,void* buf,size_t len,int flag){
	int	n = len;
	int nwrite = 0;
	while (n > 0) {
	    nwrite = send(sock, buf + len - n, n,0);
	    if (nwrite < n) {
	        if (nwrite == -1 && errno != EAGAIN) {
	            return -1;
	        }
	        break;
	    }	
	    //TODO test code
	    if(errno == EAGAIN){
    		printf("this test code,need to delete errno == EAGAIN send:%d\n",nwrite);
    	}
	    if(nwrite == 0)
	    	return -1;
 	   n -= nwrite;
	}	
	return nwrite==0?len:(len-n);
}


int socket_send(int sock,void* buf,size_t len,int flag){
	int	n = len;
	int nwrite = 0;
	while (n > 0) {
	    nwrite = write(sock, buf + len - n, n);
	    if (nwrite < n) {
	        if (nwrite == -1 && errno != EAGAIN) {
	            return -1;
	        }
	        break;
	    }	
 	   n -= nwrite;
	}
	return nwrite==0?len:(len-n);
}
