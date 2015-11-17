#ifndef LOG_H
#define LOG_H
#include<stdarg.h>

//#define Log(info) LogD(info,__FILE__,__LINE__,##__VA_ARGS__)
#define Log(format,...) LogD(format,__FILE__,__LINE__,##__VA_ARGS__)
#ifdef WIN32
	#define FILE_SPLIT '\\'
#else
	#define FILE_SPLIT '/'
#endif
#define FILE_WRITE_BUFF_LEN 4096
int centisec();
void LogV(const char *pszFmt,char* file,int line,va_list argp);
void LogD(const char *pszFmt,char* file,int line,...);


#endif /* !LOG_H */