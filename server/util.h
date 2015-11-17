#ifndef UNTIL_H
#define UNTIL_H
#ifdef WIN32
#include <windows.h>
#endif
#define MAXBUF 4096

#ifndef NULL
#define NULL 0
#endif

char* formattime(char* buffer,char* format);
char* createGUID(char* uid);
int split(char** dst, char* str, const char* spl);
int splitcount(char** dst, char* str, const char* spl);

void dump_data(void* buf,int len);
int writetofile(const char* filePath,const char* fileName,void* content,int len);
void* readtofile(const char* filePath,const char* fileName,int* ret);
unsigned long get_file_size(const char *path);
void itoa_(int i,char*string);
char* createfullpath(const char* filePath,const char* fileName,char* fullpath);
extern char* createMd5(const char* str,char* retStr);
#endif /* !UNTIL_H */