#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "log.h"
#include "sc.h"

#define MAXBUF 1024
#define WIN32


#ifdef Log
#undef Log
#endif
#ifndef Log
#define Log(format,...) NULL
#endif

#ifndef DEBUG
#define DUMP_DT(buf,len) NULL
#else
#define DUMP_DT(buf,len) dump_data(buf,len)
#endif

int splitcount(char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    n = 0;
    while( result != NULL )
    {
        n++;
        result = strtok(NULL, spl);
    }
    return n;
}
int split(char** dst, char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    if(dst == NULL)
    {
        return 0;
    }
    result = strtok(str, spl);
    n = 0;
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

void sendhelo(int sockfd,char* token)
{
    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER)+strlen(token));
    header->contentLength = 0;
    //header->tokenLength = strlen(token);
    strcpy(header->clienttoken,token);
    header->type = MESSAGE_HELO;
    header->messagetype = MESSAGE_TYPE_TXT;
    int length = sizeof(CLIENT_HEADER);//+strlen(token);
    header->totalLength = length;
    void* buff = malloc(length);
    bzero(buff,length);
    memcpy(buff,header,sizeof(CLIENT_HEADER));
    //memcpy(buff+sizeof(CLIENT_HEADER),token,strlen(token));
    int len = send(sockfd, buff, length, 0);
    printf("send helo data:\n");
    DUMP_DT(buff,len);
    free(header);
    free(buff);
    if (len > 0)
    {
        Log("msg:%s send successful，totalbytes: %d！\n", buff, len);
    }
    else
    {
        printf("msg:'%s  failed!\n", buff);
    }

}
void sendping(int sockfd,char* token)
{
    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
    header->contentLength = 0;
    //header->tokenLength = strlen(token);
    strcpy(header->clienttoken,token);
    header->type = MESSAGE_PING;
    header->messagetype = MESSAGE_TYPE_TXT;
    int length = sizeof(CLIENT_HEADER);
    header->totalLength = length;
    void* buff = malloc(length);
    bzero(buff,length);
    memcpy(buff,header,sizeof(CLIENT_HEADER));
    int len = send(sockfd, buff, length, 0);
    printf("send ping data send len=%d,size=%d:\n",len,sizeof(CLIENT_HEADER));
    DUMP_DT(buff,len);
    free(header);
    free(buff);
    if (len > 0)
        Log("msg:%s send successful，totalbytes: %d！\n", buff, len);
    else
    {
        printf("msg:'%s  failed!\n", buff);
    }

}
/**
 * time to ping server
 */
void thread(thread_struct* thread_s)
{
    while(thread_s->isexit == 0)
    {
        sendping(thread_s->sockfd,thread_s->token);
        int i = 0;
        for(i = 0; i<30; i++)
        {
            if(thread_s->isexit == 0)
                sleep(1);
            else
                return;
        }

    }
}
int g_sock;


void* client_thread(char **argv)
{

    //char totoken_temp[64]={0};    
    int isQuit = 0;
    while(isQuit == 0)
    {
        int sockfd, len;
        struct sockaddr_in dest;
        char buf[MAXBUF + 1];
        char token[MAXBUF];
        fd_set rfds;
        struct timeval tv;
        int retval, maxfd = -1;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("Socket");
            exit(errno);
        }

        bzero(&dest, sizeof(dest));
        bzero(&token,sizeof(token));
        strcpy(token,argv[3]);
        char* tempArgv = malloc(strlen(argv[4]));
        strcpy(tempArgv,argv[4]);
        int totokenLength = splitcount(argv[4],"_");
        char** totoken_temp = NULL;
        totoken_temp = malloc(totokenLength);
        int i =0;
        for(i=0; i<totokenLength; i++)
        {
            totoken_temp[i] = malloc(64);
        }
        totokenLength = split(totoken_temp,tempArgv,"_");
        free(tempArgv);
        //bzero(&totoken_temp,sizeof(totoken_temp));

        //strcpy(totoken_temp,argv[4]);
        dest.sin_family = AF_INET;
        dest.sin_port = htons(atoi(argv[2]));
        if (inet_aton(argv[1], (struct in_addr *) &dest.sin_addr.s_addr) == 0)
        {
            perror(argv[1]);
            exit(errno);
        }

        if(connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0)
        {
            perror("Connect ");
            exit(errno);
        }
		g_sock = sockfd;
        printf("connect to server port:%d...\n",dest.sin_port);

        sendhelo(sockfd,token);
        //clock_t start = clock();
        //start ping thread
        thread_struct st;
        st.sockfd = sockfd;
        st.isexit = 0;
        strcpy(st.token,token);
        pthread_t thread_id;
        pthread_mutex_init(&st.lock, NULL);
        int ret=pthread_create(&thread_id,NULL,(void *) thread,&st);
        pthread_mutex_unlock(&st.lock);
        while (1)
        {

            FD_ZERO(&rfds);
            //FD_SET(0, &rfds);
            maxfd = 0;

            FD_SET(sockfd, &rfds);
            if (sockfd > maxfd)
                maxfd = sockfd;

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);

            if (retval == -1)
            {
                printf("select error! %s", strerror(errno));
                break;
            }
            else if (retval == 0)
            {
                //printf("no msg,no key, and continue to wait…??\n");
                continue;
            }
            else
            {
                if (FD_ISSET(0, &rfds))
                {
                	/*
                    bzero(buf, MAXBUF + 1);
                    fgets(buf, MAXBUF, stdin);
                    char filepath[500]= {0};
                    int isfile = 0;
                    if (!strncasecmp(buf, "quit", 4))
                    {

                        printf("request terminal chat！\n");
                        isQuit = 1;
                        break;
                    }
                    if (!strncasecmp(buf, "exit", 4))
                    {
                        printf("request terminal chat by exit！\n");
                        break;
                    }
                    if(!strncasecmp(buf,"file",4))
                    {
                        //TODO 获得文件
                        sscanf(buf,"%*s%s",filepath);
                        printf("%s\n",filepath);
                        isfile = 1;
                    }
                    int length = -1;
                    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
                    header->contentLength = strlen(buf);

                    header->type = MESSAGE;
                    if(isfile == 0)
                        header->messagetype = MESSAGE_TYPE_TXT;
                    else
                    {
                        header->messagetype = MESSAGE_TYPE_FILE;
                    }
                    strcpy(header->clienttoken,token);
                    header->sendto = totokenLength;
                    if(header->sendto > 0)
                    {
                        length = sizeof(CLIENT_HEADER)+strlen(buf)+64*header->sendto;//+strlen(token)
                    }
                    else
                    {
                        length = sizeof(CLIENT_HEADER)+strlen(buf);//+strlen(token)
                    }
                    if(isfile == 1)
                    {
                        length += 255;
                        length -= strlen(buf);
                        length += get_file_size(filepath);
                        header->contentLength = get_file_size(filepath);
                    }
                    header->totalLength = length;
                    Log("%d,%d,%d,%d\n",header->type,header->contentLength,strlen(buf),length);

                    void* buff = malloc(length);
                    bzero(buff,length);
                    memcpy(buff,header,sizeof(CLIENT_HEADER));
                    if(isfile == 1)
                        memcpy(buff+sizeof(CLIENT_HEADER),filepath,255);

                    if(header->sendto > 0)
                    {
                        //TODO add sendto
                        int j = 0;
                        for(j=0; j<totokenLength; j++)
                        {
                            char tochar[64]= {0}; // = "bbbbbb";
                            strcpy(tochar,totoken_temp[j]);
                            if(isfile == 0)
                                memcpy(buff+sizeof(CLIENT_HEADER)+64*j,tochar,64);
                            else
                                memcpy(buff+sizeof(CLIENT_HEADER)+64*j+255,tochar,64);
                        }
                        //TODO add content message
                        if(isfile == 0)
                            memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength,buf,strlen(buf));
                        else
                        {
                            //memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,buf,strlen(buf));
                            int retx;
                            char* filecontent = readtofile(filepath,"",&retx);
                            memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,filecontent,retx);
//							memcpy(sendbuffer+sizeof(SERVER_HEADER)+255,filecontent,ret);
                            free(filecontent);
                        }
                    }
                    else
                    {
                        if(isfile == 1)
                        {
                            //memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,buf,strlen(buf));
                            int retx;
                            char* filecontent = readtofile(filepath,"",&retx);
                            memcpy(buff+sizeof(CLIENT_HEADER)+255,filecontent,retx);
                            free(filecontent);
                        }
                        else
                            memcpy(buff+sizeof(CLIENT_HEADER),buf,strlen(buf));
                    }
                    //DUMP_DT(buff,length);
                    len = 0;
                    if(length>MAXBUF)
                    {
                        while(1)
                        {
                            int tempRet = 0;
                            if((length-len)>MAXBUF)
                            {
                                tempRet = send(sockfd, buff+len, MAXBUF, 0);
                            }
                            else
                            {
                                tempRet = send(sockfd, buff+len, length-len, 0);
                                len += tempRet;
                                break;
                            }
                            len += tempRet;
                        }

                    }
                    else
                    {
                        len = send(sockfd, buff, length, 0);
                    }

                    printf("send message data:\n");
                    //DUMP_DT(buff,len);
                    free(header);
                    free(buff);
                    if (len > 0)
                    {
                        Log("msg:%s send successful，totalbytes: %d,%d！\n", buf, len,length);
                        printf("msg:%s send successful，totalbytes: %d,%d！\n", buf, len,length);
                    }
                    else
                    {
                        printf("msg:'%s  failed!\n", buf);
                        break;
                    }
                    */
                }
                else if (FD_ISSET(sockfd, &rfds))
                {
                    bzero(buf, MAXBUF + 1);
                    len = recv(sockfd, buf, MAXBUF, 0);
                    //printf("recv data recv len:%d,SERVER_HEADER len:%d\n",len,sizeof(SERVER_HEADER));
                    //DUMP_DT(buf,len);
                    SERVER_HEADER* header = malloc(sizeof(SERVER_HEADER));
                    bzero(header,sizeof(SERVER_HEADER));
                    if (len > 0)
                    {
                        if(len>=sizeof(SERVER_HEADER))
                        {
                            memcpy(header,buf,sizeof(SERVER_HEADER));
                            if(header->messageCode == MESSAGE_SUCCESS)
                            {
                                if(header->type == MESSAGE)
                                {
                                    char filename[255]= {0};
                                    void* buffers = malloc(header->contentLength);
                                    if(header->messagetype == MESSAGE_TYPE_TXT)
                                    {
                                        memcpy(buffers,buf+sizeof(SERVER_HEADER),header->contentLength);
                                        char* tempBuffers = (char*)buffers;
                                        tempBuffers[header->contentLength] = '\0';
                                        printf("Recv message is: %s\n",buffers);
                                    }
                                    else
                                    {
                                        memcpy(filename,buf+sizeof(SERVER_HEADER),255);
                                        memcpy(buffers,buf+sizeof(SERVER_HEADER)+255,header->contentLength);
                                        writetofile("/tmp",filename,buffers,header->contentLength);
                                        printf("Recv message is: %s\n",filename);
                                    }
                                    free(buffers);
                                    //return OK
                                    CLIENT_HEADER* cheader = malloc(sizeof(CLIENT_HEADER));
                                    bzero(cheader,sizeof(CLIENT_HEADER));
                                    cheader->totalLength= sizeof(CLIENT_HEADER);
                                    cheader->type = S_OK;
                                    cheader->messagetype = MESSAGE_TYPE_TXT;
                                    len = send(sockfd,cheader,sizeof(CLIENT_HEADER),0);
                                    printf("send cheader data:\n");
                                    DUMP_DT(cheader,sizeof(CLIENT_HEADER));
                                    free(cheader);

                                }
                                else
                                {

                                }
                            }
                        }
                        Log("recv:'%s, total: %d,%d \n", buf, len,header->messageCode);
                    }
                    else
                    {
                        if (len < 0)
                            printf("recv failed！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                        else
                        {
                        	printf("recv failed！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                            printf("other exit，terminal chat\n");
                        }


                        free(header);
                        break;
                        //continue;
                    }
                    free(header);
                }
            }
        }

        close(sockfd);
        pthread_mutex_init(&st.lock, NULL);
        st.isexit = 1;
        pthread_mutex_unlock(&st.lock);
        pthread_join(thread_id,0);
    }
    return 0;
}
int main(int argc, char **argv){
	
	if (argc <= 3)
    {
        printf("Usage: %s IP Port token",argv[0]);
        exit(0);
    }
    char token[64];
     bzero(&token,strlen(token));     
        strcpy(token,argv[3]);
        char* tempArgv = malloc(strlen(argv[4]));
        strcpy(tempArgv,argv[4]);
        int totokenLength = splitcount(argv[4],"_");
        char** totoken_temp = NULL;
        totoken_temp = malloc(totokenLength);
        int i =0;
        for(i=0; i<totokenLength; i++)
        {
            totoken_temp[i] = malloc(64);
        }
        totokenLength = split(totoken_temp,tempArgv,"_");
        free(tempArgv);
        
    pthread_t thr_id;
    if (pthread_create(&thr_id, NULL, (void *) client_thread, argv) != 0)
        {
            printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
                   errno, strerror(errno));
            exit(1);
        }
    //int ret=pthread_create(&thread_id,NULL,(void *) thread,&st);
    char buf[MAXBUF];
   	while(1){
		bzero(buf, MAXBUF + 1);
        fgets(buf, MAXBUF, stdin);
        char filepath[500]= {0};
        int isfile = 0;
        if (!strncasecmp(buf, "quit", 4))
        {

            printf("request terminal chat！\n");
            exit(0);
        }
        if (!strncasecmp(buf, "exit", 4))
        {
            printf("request terminal chat by exit！\n");
            break;
        }
        if(!strncasecmp(buf,"file",4))
        {
            //TODO 获得文件
            sscanf(buf,"%*s%s",filepath);
            printf("%s\n",filepath);
            isfile = 1;
        }   	
        int length = -1;
        CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
        header->contentLength = strlen(buf);

        header->type = MESSAGE;
        if(isfile == 0)
            header->messagetype = MESSAGE_TYPE_TXT;
        else
        {
            header->messagetype = MESSAGE_TYPE_FILE;
        }
        strcpy(header->clienttoken,token);
        header->sendto = totokenLength;
        if(header->sendto > 0)
        {
            length = sizeof(CLIENT_HEADER)+strlen(buf)+64*header->sendto;//+strlen(token)
        }
        else
        {
            length = sizeof(CLIENT_HEADER)+strlen(buf);//+strlen(token)
        }
        if(isfile == 1)
        {
            length += 255;
            length -= strlen(buf);
            length += get_file_size(filepath);
            header->contentLength = get_file_size(filepath);
        }
        header->totalLength = length;
        Log("%d,%d,%d,%d\n",header->type,header->contentLength,strlen(buf),length);

        void* buff = malloc(length);
        bzero(buff,length);
        memcpy(buff,header,sizeof(CLIENT_HEADER));
        if(isfile == 1)
            memcpy(buff+sizeof(CLIENT_HEADER),filepath,255);

        if(header->sendto > 0)
        {
            //TODO add sendto
            int j = 0;
            for(j=0; j<totokenLength; j++)
            {
                char tochar[64]= {0}; // = "bbbbbb";
                strcpy(tochar,totoken_temp[j]);
                if(isfile == 0)
                    memcpy(buff+sizeof(CLIENT_HEADER)+64*j,tochar,64);
                else
                    memcpy(buff+sizeof(CLIENT_HEADER)+64*j+255,tochar,64);
            }
            //TODO add content message
            if(isfile == 0)
                memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength,buf,strlen(buf));
            else
            {
                //memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,buf,strlen(buf));
                int retx;
                char* filecontent = readtofile(filepath,"",&retx);
                memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,filecontent,retx);
//							memcpy(sendbuffer+sizeof(SERVER_HEADER)+255,filecontent,ret);
                free(filecontent);
            }
        }
        else
        {
            if(isfile == 1)
            {
                //memcpy(buff+sizeof(CLIENT_HEADER)+64*totokenLength+255,buf,strlen(buf));
                int retx;
                char* filecontent = readtofile(filepath,"",&retx);
                memcpy(buff+sizeof(CLIENT_HEADER)+255,filecontent,retx);
                free(filecontent);
            }
            else
                memcpy(buff+sizeof(CLIENT_HEADER),buf,strlen(buf));
        }
        //DUMP_DT(buff,length);
        int len = 0;
        if(length>MAXBUF)
        {
            while(1)
            {
                int tempRet = 0;
                if((length-len)>MAXBUF)
                {
                    tempRet = send(g_sock, buff+len, MAXBUF, 0);
                }
                else
                {
                    tempRet = send(g_sock, buff+len, length-len, 0);
                    len += tempRet;
                    break;
                }
                len += tempRet;
            }

        }
        else
        {
            len = send(g_sock, buff, length, 0);
        }
		printf("send message！errno:%d，error msg: '%s'\n", errno, strerror(errno));
        //DUMP_DT(buff,len);
        free(header);
        free(buff);
        if (len > 0)
        {
            Log("msg:%s send successful，totalbytes: %d,%d！\n", buf, len,length);
            printf("msg:%s send successful，totalbytes: %d,%d！\n", buf, len,length);
        }
        else
        {
            printf("msg:'%s  failed!\n", buf);
            break;
        }

 	}
    pthread_join(thr_id,NULL);
}

