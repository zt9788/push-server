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
#include <sys/stat.h>
#include "com_example_test_ServiceCalled.h"
#include <android/log.h>
//#include "sc.h"

#define   LOG_TAG    "LOG_TEST"
#define   LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define   LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


#define MAXBUF 4096
#define WIN32


#ifdef Log
#undef Log
#endif
#ifndef Log
#define Log(format,...) NULL
#endif

typedef struct CLIENT_HEADER
{
    int type;
    //int tokenLength;
    int contentLength;
    int userid;
    char clienttoken[64];
    int sendto;
    int clienttype;/*0=android,1=ios,2=web*/
    int totalLength;
    int messagetype;/*0=txt,1=img,2=audio,3=video,4=file*/
//	char fileName[255];
} CLIENT_HEADER;

typedef struct SERVER_HEADER
{
    int messageCode;
    int type;
    int messagetype;
    int contentLength;
    int totalLength;
    char fromtoken[64];
} SERVER_HEADER;


//message content type
#define MESSAGE_TYPE_TXT  0xF11
#define MESSAGE_TYPE_IMG  0xF22
#define MESSAGE_TYPE_AUDIO  0xF22
#define MESSAGE_TYPE_VIDEO  0xF22
#define MESSAGE_TYPE_FILE  0xF22
//message type
#define MESSAGE_HELO 0xA0
#define MESSAGE_PING 0xA1
#define MESSAGE 	 0xA2
#define MESSAGE_SERVER 0xA3
#define MESSAGE_BYE 0xA4
#define MESSAGE_YES 0xA5
#define S_OK		0xA6
//response type
#define MESSAGE_SUCCESS 0x01
#define MESSAGE_FILDE 0x00


typedef struct thread_struct
{
    int sockfd;
    pthread_mutex_t lock;
    int isexit;
    char token[255];
} thread_struct;

typedef struct sendlist_x
{

    int len;
    unsigned char* buff;
    char* messageid;
    struct sendlist_x* next;
} sendlist_x_t;

static int g_isstart = 1;
static int g_sockfd = -1;
static char g_token[64] = {0};
static char g_serverip[20]= {0};
static int g_serverport = -1;
//for ping thread
pthread_t thread_id2 = -1;
//for send thread quene
pthread_t thread_id3 = -1;
//for recv thread
pthread_t thread_id = -1;
JNIEnv* g_env;
jobject g_thiz;
JavaVM *g_jvm;
jobject g_obj;
static sendlist_x_t* sendlist = NULL;
pthread_mutex_t fd_lock;
pthread_cond_t  queue_send;
pthread_mutex_t send_lock;
int sleep_time_out = 1;
thread_struct st;
void* sendMessage(JNIEnv* env,void* args);
void* sendMessage_thead(void* args);

unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0)
    {
        return filesize;
    }
    else
    {
        filesize = statbuff.st_size;
    }
    return filesize;
}
void* readtofile(const char* filePath,const char* fileName,int* ret)
{

    char fullpath[500]= {0};
    int length = strlen(filePath);
    if(length != 0 && filePath != NULL && filePath[0] != 0)
    {
        if(filePath[length] == '\\' || filePath[length] == '/')
        {
            sprintf(fullpath,"%s%s%s",fullpath,filePath,fileName);
        }
        else
        {
            sprintf(fullpath,"%s%s/%s",fullpath,filePath,fileName);
        }
    }
    else
    {
        sprintf(fullpath,"%s",fileName);
    }

    long len = get_file_size(fullpath);
    LOGI("read file:%s:%d\n",fullpath,len);
    FILE * flogx=fopen(fullpath,"rb");
    int sum = 0;
    void* buf = malloc(len);
    if (NULL!=flogx)
    {
        //fprintf(flog,"%s %s.%s File:%s(%d) %s",datestr,timestr,ms10,file,line,logstr);
        //ret = fwrite(content,len,1,flog);
        *ret = fread(buf,1,len,flogx);
        fclose(flogx);
    }
    LOGI("write file filed:%s",fullpath);
    return buf;
}

int writetofile(const char* filePath,const char* fileName,void* content,int len)
{
    char fullpath[500]= {0};
    int length = strlen(filePath);
    if(length != 0 && filePath != NULL && filePath[0] != 0)
    {
        if(filePath[length] == '\\' || filePath[length] == '/')
        {
            sprintf(fullpath,"%s%s%s",fullpath,filePath,fileName);
        }
        else
        {
            sprintf(fullpath,"%s%s/%s",fullpath,filePath,fileName);
        }
    }
    else
    {
        sprintf(fullpath,"%s",fileName);
    }

    FILE * flog=fopen(fullpath,"a");
    int ret = -1;
    if (NULL!=flog)
    {
        //fprintf(flog,"%s %s.%s File:%s(%d) %s",datestr,timestr,ms10,file,line,logstr);
        ret = fwrite(content,len,1,flog);
        fclose(flog);
    }
    else
    {
        LOGI("write file filed:%s",fullpath);
    }
    return ret;

}
jstring stoJstring(JNIEnv* env,const char* pat);
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

void *GetFilename(char *p,char spli,char* ret)
{
    int x = strlen(p);
    char ch = spli;
    char *q = strrchr(p,ch) + 1;
    if(strlen(q) > 0)
    {
        strcpy(ret,q);
        return q;
    }
    else
    {
        bzero(ret,strlen(ret));
        return ret;
    }
}

void sendhelo(int sockfd,char* token)
{

    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
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
    pthread_mutex_lock(&fd_lock);
    int len = send(sockfd, buff, length, 0);
    pthread_mutex_unlock(&fd_lock);
    LOGI("send helo data:\n");
    //DUMP_DT(buff,len);
    free(header);
    free(buff);
    if (len > 0)
    {
        LOGI("msg:%s send successful，totalbytes: %d,%d！\n", buff, len,length);
    }
    else
    {
        LOGI("msg:'%s  failed!\n", buff);
    }

}
void sendping(int sockfd,char* token)
{
    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
    header->contentLength = 0;
    //header->tokenLength = strlen(token);
    header->messagetype = MESSAGE_TYPE_TXT;
    strcpy(header->clienttoken,token);
    header->type = MESSAGE_PING;
    int length = sizeof(CLIENT_HEADER);//+strlen(token);
    header->totalLength = length;
    void* buff = malloc(length);
    bzero(buff,length);
    memcpy(buff,header,sizeof(CLIENT_HEADER));
    pthread_mutex_lock(&fd_lock);
    int len = send(sockfd, buff, length, 0);
    pthread_mutex_unlock(&fd_lock);
    free(header);
    free(buff);
    if (len > 0)
        Log("msg:%s send successful，totalbytes: %d,%d！\n", buff, len,length);
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
    JNIEnv* env;
    (*g_jvm)->AttachCurrentThread(g_jvm,&env, NULL);
    while(thread_s->isexit == 0)
    {
        sendping(thread_s->sockfd,g_token);//thread_s->token);
        int i=0;
        int exit = 0;
        for(i=0; i<60; i++)
        {
            if(thread_s->isexit != 0)
            {
                exit = 1;
                break;
            }
            sleep(1);
        }
        if(exit == 1)
            break;

    }
    (*g_jvm)->DetachCurrentThread(g_jvm);
}

char* getFileName(char* fullpath)
{
    int len;
    char *current=NULL;

    len=strlen(fullpath);
    for (; len>0; len--) //从最后一个元素开始找.直到找到第一个'/'
    {
        if(fullpath[len]=='/')
        {
            current=&fullpath[len+1];
            return current;
        }
    }
    //for windows file
    if(current == NULL)
    {
        for (; len>0; len--) //从最后一个元素开始找.直到找到第一个'/'
        {
            if(fullpath[len]=='\\')
            {
                current=&fullpath[len+1];
                return current;
            }
        }
    }
    return fullpath;
}
void callJavaMethod(JNIEnv *env,char* methodName,char* args,char* buffers)
{
    jclass jcls = (*env)->GetObjectClass(env,g_obj);
    jmethodID method1 = (*env)->GetMethodID(env,jcls,methodName,args);
    if ((*env)->ExceptionCheck(env))
    {
        //return ERR_FIND_CLASS_FAILED;
        LOGI("error create method1");
    }
    jstring message = stoJstring(env,( char*)buffers);
    //LOGI("%s",buffers);
    (*env)->CallVoidMethod(env,g_obj,method1,message);
    if ((*env)->ExceptionCheck(env))
    {
        LOGI("error create CallStaticVoidMethod");
    }
}
void callJavaMethodEx(JNIEnv *env,char* methodName,char* args,char* messageid,int responseCode,char* error)
{
	//TODO 2
	LOGI("callJavaMethodEx,%d,%d",env,g_obj);
    jclass jcls = (*env)->GetObjectClass(env,g_obj);
    LOGI("callJavaMethodEx,%d",jcls);
    jmethodID method1 = (*env)->GetMethodID(env,jcls,methodName,args);
    if ((*env)->ExceptionCheck(env))
    {
        //return ERR_FIND_CLASS_FAILED;
        LOGI("error create method1");
        return;
    }
    LOGI("callJavaMethodEx 333");
    jstring jerror = stoJstring(env,( char*)error);
    LOGI("callJavaMethodEx 4");
    jstring jmessageid = stoJstring(env,(char*)messageid);
    LOGI("callJavaMethodEx 5");
    //LOGI("%s",buffers);
    (*env)->CallVoidMethod(env,g_obj,method1,jmessageid,responseCode,jerror);
    LOGI("callJavaMethodEx 6");
    if ((*env)->ExceptionCheck(env))
    {
        LOGI("error create CallStaticVoidMethod");
    }
}
void sendMessageResponse(JNIEnv *env,char* messageid,int responseCode,char* error){
	callJavaMethodEx(env,"sendMessageResponse","(Ljava/lang/String;ILjava/lang/String;)V",messageid,responseCode,error);
}
void recvData(JNIEnv *env,char* message)
{
    callJavaMethod(env,"recvData","(Ljava/lang/String;)V",message);
}
void recvFile(JNIEnv *env,char* filePath)
{
    callJavaMethod(env,"recvFile","(Ljava/lang/String;)V",filePath);
}
void thread_recv(thread_struct* thread_s)
{
    JNIEnv *env;
    (*g_jvm)->AttachCurrentThread(g_jvm,&env, NULL);

    int sockfd, len;
    struct sockaddr_in dest;
    char buf[MAXBUF + 1];
    char token[MAXBUF];
    fd_set rfds;
    struct timeval tv;
    int retval, maxfd = -1;
    unsigned char b = 1;
    pthread_t thread_id = -1;
    thread_struct st;
    LOGI("in recv token %s:",thread_s->token);
    strcpy(token,g_token);

    while(g_isstart == 1)
    {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            LOGI("Create Socket error");
            (*g_jvm)->DetachCurrentThread(g_jvm);
            return ;
        }
        bzero(&dest, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(g_serverport);
        if (inet_aton(g_serverip, (struct in_addr *) &dest.sin_addr.s_addr) == 0)
        {
            LOGI("%d can not connection ",g_serverip);
            (*g_jvm)->DetachCurrentThread(g_jvm);
            return;
        }
        int optval = 1;
        int optlen = sizeof(optval);
        if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
        {
            perror("setsockopt()");
            close(sockfd);
            // exit(EXIT_FAILURE);
            return;
        }
        LOGI("connect to %s %d",g_serverip,g_serverport);
        while(connect(sockfd, (struct sockaddr *) &dest, sizeof(dest)) != 0)
        {
            LOGI("Connect error");
            sleep_time_out = sleep_time_out*2;
//			sleep(1);
            int j = 0;
            for(j=0; j<sleep_time_out; j++)
            {
                if(thread_s->isexit != 0)
                {
                    return;
                }
                sleep(1);                
            }
        }
        g_sockfd = sockfd;
        sleep_time_out = 1;

        LOGI("start helo%d",sockfd);
        LOGI("token is :%s",token);
        sendhelo(sockfd,token);

//		//clock_t start = clock();
//		//start ping thread
//
//
        st.sockfd = sockfd;
        st.isexit = 0;
        strcpy(st.token,token);
        //TODO
        int kill_rc = pthread_kill(thread_id2,0);
        pthread_mutex_init(&st.lock, NULL);
        if(kill_rc == ESRCH)
        {
            pthread_mutex_lock(&st.lock);
            int ret=pthread_create(&thread_id2,NULL,(void *) thread,&st);
            pthread_mutex_unlock(&st.lock);
        }
        kill_rc = pthread_kill(thread_id3,0);
        if(kill_rc == ESRCH)
        {
            int ret=pthread_create(&thread_id3,NULL,(void *) sendMessage_thead,NULL);
        }
//
        while (g_isstart == 1)
        {

            FD_ZERO(&rfds);
            maxfd = 0;
            FD_SET(0, &rfds);
            FD_SET(sockfd, &rfds);
            if (sockfd > maxfd)
                maxfd = sockfd;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            retval = select(maxfd + 1, &rfds, NULL, NULL, &tv);
            if (retval == -1)
            {
                LOGI("retval break");
                break;
            }
            else if (retval == 0)
            {
                //LOGI("retval continue");
                continue;
            }
            else
            {
                //LOGI("socketfd %d",sockfd);
                if(FD_ISSET(sockfd, &rfds))// (FD_ISSET(0, &rfds))
                {
                    bzero(buf, MAXBUF + 1);
                    pthread_mutex_lock(&fd_lock);
                    len = recv(sockfd, buf, MAXBUF, 0);
                    LOGI("2eventList error！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                    pthread_mutex_unlock(&fd_lock);
                    SERVER_HEADER* header = malloc(sizeof(SERVER_HEADER));
                    bzero(header,sizeof(SERVER_HEADER));
                    LOGI("zhege shi changdu :%d",len);
                    LOGI("Recv Message %s,%d",buf,len);
                    if (len > 0)
                    {
                        if(len>=sizeof(SERVER_HEADER))
                        {
                            memcpy(header,buf,sizeof(SERVER_HEADER));
                            if(header->messageCode == MESSAGE_SUCCESS)
                            {
                                //read for all
                                int sum = len;
                                void* tempBuffer = NULL;
                                tempBuffer = malloc(header->totalLength);
                                memcpy(tempBuffer,buf,len);
                                LOGI("need recv length=%d,now is=%d",header->totalLength,len);
                                if(header->totalLength>MAXBUF && header->totalLength > len)
                                {
                                    pthread_mutex_lock(&fd_lock);
                                    while(1)
                                    {
                                        int tempret = 0;
                                        bzero(buf,MAXBUF);
                                        if((header->totalLength-sum)>MAXBUF)
                                        {
                                            tempret = recv(sockfd,buf,MAXBUF,0);
                                            if(tempret <0)
                                            {
                                                if(tempBuffer != NULL)
                                                {
                                                    LOGI("free1");
                                                    free(tempBuffer);
                                                    tempBuffer = NULL;
                                                }
                                                pthread_mutex_unlock(&fd_lock);
                                                goto recv_error;
                                            }
                                        }
                                        else
                                        {
                                            //LOGI("haiyao=%d,zongcd=%d",header->totalLength-sum,header->totalLength);
                                            tempret = recv(sockfd,buf,header->totalLength-sum,0);
                                            //LOGI("daodi shuicuole %d",tempret);
                                            if(tempret <0)
                                            {
                                                if(tempBuffer != NULL)
                                                {
                                                    LOGI("free2");
                                                    free(tempBuffer);
                                                    tempBuffer = NULL;
                                                }
                                                pthread_mutex_unlock(&fd_lock);
                                                goto recv_error;
                                            }
                                            memcpy(tempBuffer+sum,buf,tempret);
                                            sum+=tempret;
                                            if(header->totalLength-sum == 0)
                                                break;
                                            else
                                                continue;
                                        }
                                        //pthread_mutex_unlock(&fd_lock);
                                        memcpy(tempBuffer+sum,buf,tempret);
                                        sum += tempret;
                                        LOGI("need recv length=%d,now is=%d",header->totalLength,sum);
                                    }
                                    //buf = tempBuffer;
                                    pthread_mutex_unlock(&fd_lock);
                                }

                                if(header->type == MESSAGE)
                                {
                                    char filename[255]= {0};
                                    void* buffers = NULL;
                                    if(header->contentLength>0)
                                        buffers = malloc(header->contentLength+1);

                                    if(header->messagetype == MESSAGE_TYPE_TXT)
                                    {
                                        memcpy(buffers,tempBuffer+sizeof(SERVER_HEADER),header->contentLength);
                                        char* tempBuffers = (char*)buffers;
                                        tempBuffers[header->contentLength] = '\0';
                                        printf("Recv message is: %s,%d\n",buffers,len);
                                        recvData(env,buffers);
                                    }
                                    else
                                    {
                                        unsigned char fn[255];
                                        memcpy(filename,tempBuffer+sizeof(SERVER_HEADER),255);

                                        memcpy(fn,tempBuffer+sizeof(SERVER_HEADER),255);
                                        GetFilename(fn,'\\',filename);
                                        if(strlen(filename) == 0)
                                            GetFilename(fn,'/',filename);
                                        memcpy(buffers,tempBuffer+sizeof(SERVER_HEADER)+255,header->contentLength);
                                        //writetofile("/sdcard",filename,buffers,header->contentLength);
                                        char fullname[500]= {0};
                                        sprintf(fullname,"%s/","/storage/sdcard0");
                                        sprintf(fullname,"%s%s",fullname,filename);
                                        LOGI("full name:%s",fullname);
                                        LOGI("file name:%s",filename);
                                        //char tempName[255]={0};
                                        //strcpy(tempName,filename);
                                        char* tempName = (char*)fn;
                                        LOGI("tempName name:%s",tempName);
                                        int leng;
                                        writetofile("/storage/sdcard0",filename,buffers,header->contentLength);
                                        recvFile(env,fullname);
                                    }
                                    if(buffers != NULL)
                                    {
                                        LOGI("free5");
                                        free(buffers);
                                        buffers = NULL;
                                    }
                                    if(tempBuffer != NULL)
                                    {
                                        LOGI("free6");
                                        free(tempBuffer);
                                        tempBuffer = NULL;
                                    }

                                    CLIENT_HEADER* cheader = malloc(sizeof(CLIENT_HEADER));
                                    bzero(cheader,sizeof(CLIENT_HEADER));
                                    cheader->totalLength= sizeof(CLIENT_HEADER);
                                    cheader->type = S_OK;
                                    cheader->messagetype = MESSAGE_TYPE_TXT;
                                    pthread_mutex_lock(&fd_lock);
                                    len = send(sockfd,cheader,sizeof(CLIENT_HEADER),0);
                                    LOGI("3eventList error！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                                    pthread_mutex_unlock(&fd_lock);
                                    LOGI("send cheader data:\n");
                                    //DUMP_DT(cheader,sizeof(CLIENT_HEADER));
                                    free(cheader);
                                }
                                else
                                {

                                }
                            }
                        }
                    }
                    else
                    {
recv_error:
                        if (len < 0)
                        {
                            LOGI("len<0");
                            LOGI("recv failed！errno:%d，error msg: '%s'\n", errno, strerror(errno));
                        }
                        else
                        {
                            LOGI("other exit，terminal chat\n");
                        }
                        free(header);
                        break;
                    }
                    free(header);
                }

            }
        }
    }
    LOGI("connect dis connect");
    (*g_jvm)->DetachCurrentThread(g_jvm);
}

jstring stoJstring(JNIEnv* env,const char* pat)//change type char* into string
{
    jclass strClass = (*env)->FindClass(env,"java/lang/String");
    jmethodID ctorID = (*env)->GetMethodID(env,strClass, "<init>", "([BLjava/lang/String;)V");
    jbyteArray bytes = (*env)->NewByteArray(env,strlen((char*)pat));
    (*env)->SetByteArrayRegion(env,bytes, 0, strlen(pat), (jbyte*)pat);
    jstring encoding = (*env)->NewStringUTF(env,"utf-8");
    LOGI("yes stoJstring");
    return (jstring)(*env)->NewObject(env,strClass, ctorID, bytes, encoding);
}

JNIEXPORT void JNICALL Java_com_example_test_ServiceCalled_start
(JNIEnv *env, jobject jobj, jstring sip, jint sport, jstring mytoken)
{
    LOGI("call start method");
    (*env)->GetJavaVM(env,&g_jvm);
    g_env = env;
    g_obj = (*env)->NewGlobalRef(env,jobj);
    LOGI("obj:%d",g_obj);
    //LOGI("class:%s",jcls);
    char * serverips = (char *) (*env)->GetStringUTFChars(env, sip, NULL);
    char serverip[50] = {0};
    strcpy(serverip,serverips);
    strcpy(g_serverip,serverip);
    (*env)->ReleaseStringUTFChars(env, sip, serverips);
    int serverport = sport;
    g_serverport = sport;

    char* tokens = (char *) (*env)->GetStringUTFChars(env, mytoken, NULL);
    LOGI("tokens %s:",tokens);
    char token[64]= {0};
    strcpy(token,tokens);
    strcpy(g_token,tokens);
    (*env)->ReleaseStringUTFChars(env, mytoken, tokens);
    LOGI("g token %s:",g_token);
    g_isstart = 1;
    pthread_cond_init(&queue_send, NULL);
    pthread_mutex_init(&fd_lock, NULL);
    //st.sockfd = sockfd;
    st.isexit = 0;
    strcpy(st.token,g_token);
    //pthread_t thread_id;
    int kill_rc = pthread_kill(thread_id,0);
    int ret2 = -1;
    if(kill_rc == ESRCH)
        ret2 = pthread_create(&thread_id,NULL,(void *) thread_recv,&st);
    //return b;


}
void stopService()
{
    g_isstart = 0;
    close(g_sockfd);
    pthread_mutex_lock(&st.lock);
    st.isexit = 1;
    pthread_mutex_unlock(&st.lock);
    pthread_mutex_unlock(&fd_lock);
    pthread_cond_signal(&queue_send);
    pthread_join(thread_id,0);
    pthread_join(thread_id2,0);
    pthread_join(thread_id3,0);
}

JNIEXPORT void JNICALL Java_com_example_test_ServiceCalled_stop
(JNIEnv *env, jclass cls)
{
    stopService();
}
typedef struct sendmessage_stuct{
	JNIEnv* env;
	void* args;
}sendmessage_stuct_t;
void* sendMessage_thead(void* args)
{
	//pthread_detach(pthread_self());
    JNIEnv *env;
    (*g_jvm)->AttachCurrentThread(g_jvm,&env, NULL);
    LOGI("env 788,%d",env);
    sendmessage_stuct_t sst;
    sst.env = env;
    sst.args = args;
    void* ret = sendMessage(env,&sst);
    (*g_jvm)->DetachCurrentThread(g_jvm);
    return ret;
}

void* sendMessage(JNIEnv* env,void* args)
{
	sendmessage_stuct_t* sst = args;
	LOGI("env 800,%d",env);
	//JNIEnv* env = (JNIEnv*)sst->env;
    sendlist_x_t* head = NULL;//sendlist;
    while(1)
    {
    	LOGI("qidongle");
        pthread_mutex_lock(&send_lock);
        
        	LOGI("kaishidengdai");
            pthread_cond_wait(&queue_send, &send_lock);
            LOGI("jinrule");
            head = sendlist;
         while(head)
        {
            sendlist = (sendlist_x_t*)head->next;            
            int len = 0;
            int length = head->len;
            void* buff = head->buff;
            
            LOGI("total filesize %d",length);
            pthread_mutex_lock(&fd_lock);
            if(length>MAXBUF)
            {
                while(1)
                {
                    int tempRet = 0;
                    if((length-len)>MAXBUF)
                    {
                        tempRet = send(g_sockfd, buff+len, MAXBUF, 0);
                        LOGI("send len %d,%d,%d",tempRet,len,length-len);
                        if(tempRet < 0)
                        {
                            pthread_mutex_unlock(&fd_lock);
                            goto send_error2;
                        }
                    }
                    else
                    {
                        tempRet = send(g_sockfd, buff+len, length-len, 0);
                        LOGI("send len %d,%d,%d",tempRet,len,length-len);
                        if(tempRet < 0)
                        {
                            pthread_mutex_unlock(&fd_lock);
                            goto send_error2;
                        }
                        len += tempRet;
                        break;
                    }
                    len += tempRet;
                }

            }
            else
            {
                len = send(g_sockfd, buff, length, 0);
                LOGI("send len %d",len);
                if(len < 0)
                {
                    //pthread_mutex_unlock(&fd_lock);
                    goto send_error2;
                }
            }
            //recv response
            unsigned char rebuf[MAXBUF];
            len = recv(g_sockfd,rebuf,MAXBUF,0);
            if(len<0){
            	goto send_error2;
            }
send_error2:

			pthread_mutex_unlock(&fd_lock);
            pthread_mutex_unlock(&send_lock);
            //call java function to show the message is send ok
            if(len<0)
				sendMessageResponse(env,head->messageid,0,"send error");
			else
				sendMessageResponse(env,head->messageid,1,"");
            sendlist_x_t* temp = head;
            head = head->next;
            free(temp->buff);
            free(temp->messageid);
            free(temp);

            LOGI("1eventList error！errno:%d，error msg: '%s'\n", errno, strerror(errno));
        }
    }
    return NULL;
}
/*
 * Class:     com_example_test_ServiceCalled
 * Method:    sendMessageNative
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/util/List;)V
 */
JNIEXPORT jboolean JNICALL Java_com_example_test_ServiceCalled_sendMessageNative
(JNIEnv *env, jobject jobj,
		jstring jmyDriveid,
		jstring jmessageid,
		jint types,
		jstring jmessage,
		jobject jsendtoDriviceid)
{
    unsigned char b = 1;

    int type = types;
    char * tempStr = (char *) (*env)->GetStringUTFChars(env, jmessage, NULL);
    if(tempStr == NULL || strlen(tempStr) == 0)
    {
        (*env)->ReleaseStringUTFChars(env, jmessage, tempStr);
        return b;
    }
    char* message = malloc(strlen(tempStr)+1);
    //LOGI("malloc buf:%dbyte",strlen(tempStr));
    strcpy(message,tempStr);
    LOGI("%s",message);
    (*env)->ReleaseStringUTFChars(env, jmessage, tempStr);
    char* tmessageid = (char *) (*env)->GetStringUTFChars(env, jmessageid, NULL);
    if(tmessageid == NULL || strlen(tmessageid) == 0)
	{
		(*env)->ReleaseStringUTFChars(env, jmessageid, tmessageid);
		return b;
	}
    char* messageid = malloc(strlen(tmessageid));
    strcpy(messageid,tmessageid);
    (*env)->ReleaseStringUTFChars(env, jmessageid, tmessageid);
    //LOGI("%s",tempStr);
    int len = -1;


    //class ArrayList
    jclass cls_arraylist = (*env)->GetObjectClass(env,jsendtoDriviceid);
    //method in class ArrayList
    jmethodID arraylist_get = (*env)->GetMethodID(env,cls_arraylist,"get","(I)Ljava/lang/Object;");
    jmethodID arraylist_size = (*env)->GetMethodID(env,cls_arraylist,"size","()I");
    jint lenth = (*env)->CallIntMethod(env,jsendtoDriviceid,arraylist_size);

    int i = 0;
    int length = -1;
    char filepath[500]= {0};
    CLIENT_HEADER* header = malloc(sizeof(CLIENT_HEADER));
    header->contentLength = strlen(message);
    header->type = MESSAGE;
    header->messagetype = type;
    strcpy(header->clienttoken,g_token);
    header->sendto = lenth;

    if(header->sendto > 0)
    {
        length = sizeof(CLIENT_HEADER)+strlen(message)+64*header->sendto;//+strlen(token)
    }
    else
    {
        length = sizeof(CLIENT_HEADER)+strlen(message);//+strlen(token)
    }
    if(type != MESSAGE_TYPE_TXT)
    {
        length += 255;
        length -= strlen(message);
        length += get_file_size(message);
        LOGI("file size=%d",get_file_size(message));
        header->contentLength = get_file_size(message);
    }
    header->totalLength = length;
    void* buff = malloc(length);
    bzero(buff,length);
    memcpy(buff,header,sizeof(CLIENT_HEADER));
    if(type != MESSAGE_TYPE_TXT)
    {
        char fileName[255]= {0};
        GetFilename(message,'/',fileName);
        LOGI("send filename %s",fileName);
        memcpy(buff+sizeof(CLIENT_HEADER),fileName,255);
    }
    LOGI("1");
    if(header->sendto > 0)
    {
        int j = 0;
        for(j=0; j<lenth; j++)
        {
            char tochar[64]= {0};
            jstring jtoken = (*env)->CallObjectMethod(env,jsendtoDriviceid,arraylist_get,j);
            char* totoken_temp =   (char *) (*env)->GetStringUTFChars(env, jtoken, NULL);
            strcpy(tochar,totoken_temp);
            (*env)->ReleaseStringUTFChars(env, jtoken, totoken_temp);
            if(type == MESSAGE_TYPE_TXT)
                memcpy(buff+sizeof(CLIENT_HEADER)+64*j,tochar,64);
            else
                memcpy(buff+sizeof(CLIENT_HEADER)+64*j+255,tochar,64);
        }
        if(type == MESSAGE_TYPE_TXT)
            memcpy(buff+sizeof(CLIENT_HEADER)+64*lenth,message,strlen(message));
        else
        {
            int retlen;
            char* content = readtofile("",message,&retlen);
            memcpy(buff+sizeof(CLIENT_HEADER)+64*lenth+255,content,retlen);
            free(content);
        }
    }
    else
    {
        if(type == MESSAGE_TYPE_TXT)
            memcpy(buff+sizeof(CLIENT_HEADER),message,strlen(message));
        else
        {
            int retlen;
            char* content = readtofile(message,"",&retlen);
            memcpy(buff+sizeof(CLIENT_HEADER)+255,content,retlen);
            free(content);
        }
    }
    LOGI("2");
    //len = send(g_sockfd, buff, length, 0);

    if(sendlist == NULL)
    {
        sendlist = (struct sendlist_x_t*)malloc(sizeof(sendlist_x_t));
        sendlist->next = NULL;
        sendlist->buff= buff;
        sendlist->len = length;
        sendlist->messageid = messageid;
    }
    else
    {
        sendlist_x_t* work = (struct sendlist_x_t*)malloc(sizeof(sendlist_x_t));
        work->next = NULL;
        work->buff= buff;
        work->len = length;
        work->messageid = messageid;
        sendlist_x_t* head = sendlist;
        while(head->next)
        {
            head = head->next;
        }
        head->next = work;
    }
send_error:

    LOGI("send len is:%d,%d:%d\n",g_sockfd,len,length);
    LOGI("error！errno:%d，error msg: '%s'\n", errno, strerror(errno));
    free(header);
    //free(message);
    pthread_cond_signal(&queue_send);
    pthread_mutex_unlock(&send_lock);
    //free(buff);
    (*env)->DeleteLocalRef(env,cls_arraylist);
    if (len > 0)
    {
        return (jboolean)b;
    }
    else
    {
        b=0;
        return (jboolean)b;
    }
}

//	len = 0;
//	LOGI("total filesize %d",length);
//	pthread_mutex_lock(&fd_lock);
//	if(length>MAXBUF){
//		while(1){
//			int tempRet = 0;
//			if((length-len)>MAXBUF){
//				tempRet = send(g_sockfd, buff+len, MAXBUF, 0);
//				LOGI("send len %d,%d,%d",tempRet,len,length-len);
//				if(tempRet < 0){
//					pthread_mutex_unlock(&fd_lock);
//					goto send_error;
//				}
//			}else{
//				tempRet = send(g_sockfd, buff+len, length-len, 0);
//				LOGI("send len %d,%d,%d",tempRet,len,length-len);
//				if(tempRet < 0) {
//					pthread_mutex_unlock(&fd_lock);
//					goto send_error;
//				}
//				len += tempRet;
//				break;
//			}
//			len += tempRet;
//		}
//
//	}
//	else{
//		len = send(g_sockfd, buff, length, 0);
//		LOGI("send len %d",len);
//		if(len < 0){
//			pthread_mutex_unlock(&fd_lock);
//			goto send_error;
//		}
//	}
//	pthread_mutex_unlock(&fd_lock);
