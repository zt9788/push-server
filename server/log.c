#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#define  CRITICAL_SECTION   pthread_mutex_t

#endif
#define  _vsnprintf         vsnprintf
#include <sys/stat.h>
//Log{
#define MAXLOGSIZE 100000000
#define DEBUGFMT  "File:%s(%d)-%s: "
#define DEBUGARGS __FILE__,__LINE__,__FUNCTION__
#define ARRSIZE(x) (sizeof(x)/sizeof(x[0]))
#include <time.h>
#include <stdarg.h>
#include "log.h"
#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif
char logfilename1[]="log.log";
char logfilename2[]="MyLog2.log";
char logstr[16000];
char datestr[16];
char timestr[16];
char ms10[3];
CRITICAL_SECTION cs_log;
FILE *flog;

int centisec()
{
#ifdef WIN32
    return ((GetTickCount()%1000L)/10)%100;
#else
    struct timeval tv;

    if (!gettimeofday(&tv,NULL))
    {
        return ((tv.tv_usec%1000000L)/10000)%100;
    }
    else
    {
        return 0;
    }
#endif
}
#ifdef WIN32
void Lock(CRITICAL_SECTION *l)
{
    EnterCriticalSection(l);
}
void Unlock(CRITICAL_SECTION *l)
{
    LeaveCriticalSection(l);
}
#else
void Lock(CRITICAL_SECTION *l)
{
    pthread_mutex_lock(l);
}
void Unlock(CRITICAL_SECTION *l)
{
    pthread_mutex_unlock(l);
}
#endif
void LogV(const char *pszFmt,char* file,int line,va_list argp)
{
    struct tm *now;
    time_t aclock;
    if (NULL==pszFmt||0==pszFmt[0]) return;
    if (-1==_vsnprintf(logstr,ARRSIZE(logstr),pszFmt,argp)) logstr[ARRSIZE(logstr)-1]=0;
    time(&aclock);
    now=localtime(&aclock);
    sprintf(datestr,"%04d-%02d-%02d",now->tm_year+1900,now->tm_mon+1,now->tm_mday);
    sprintf(timestr,"%02d:%02d:%02d",now->tm_hour     ,now->tm_min  ,now->tm_sec );
    sprintf(ms10,"%02d",centisec());
    printf("%s %s.%s %s(%d) %s ",datestr,timestr,ms10,file,line,logstr);
    flog=fopen(logfilename1,"a");
    if (NULL!=flog)
    {
        fprintf(flog,"%s %s.%s %s(%d) %s",datestr,timestr,ms10,file,line,logstr);
        if (ftell(flog)>MAXLOGSIZE)
        {
            fclose(flog);
            if (rename(logfilename1,logfilename2))
            {
                remove(logfilename2);
                rename(logfilename1,logfilename2);
            }
            flog=fopen(logfilename1,"a");
            if (NULL==flog) return;
        }
        fclose(flog);
    }
}
void dump_data(void* buf,int len)
{
    char tmp[3]= {0};
    char bufx[1024000]= {0};
    char* xx = (char*)buf;
    int i = 0;
    for( i=0; i<len; i++ )
    {
        unsigned char x = (unsigned char)xx[i];
        sprintf(tmp,"%02X ",  x);

        if(i%8 == 0)
            strcat(bufx,"  ");
        if(i%24 == 0)
            strcat(bufx,"\n");
        strcat(bufx,tmp);
    }
    printf("the length is:%d%s\n",len,bufx);
}
void LogD(const char *pszFmt,char* file,int line,...)
{
    va_list argp;
    Lock(&cs_log);
//   va_start(argp,pszFmt);
    va_start(argp,line);
    LogV(pszFmt,file,line,argp);
    va_end(argp);
    Unlock(&cs_log);
}
//Log}
