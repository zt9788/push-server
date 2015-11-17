#include <time.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "util.h"
#include <sys/stat.h>
#ifndef WIN32
	#include <uuid/uuid.h>
#endif 
#ifdef CHECKMEM
	#include "leak_detector_c.h"
#endif 
#define GUID_LEN 64
#define FILE_WRITE_BUFF_LEN 4096
char* formattime(char* buffer,char* format){
	if(buffer == NULL) return NULL;
	//if(strlen(buffer)<128) return NULL;
 	time_t rawtime;
    struct tm * timeinfo;
    char buffers [128];
 
    time (&rawtime);
    timeinfo = localtime (&rawtime);
	if(format == NULL)
		strftime (buffers,sizeof(buffers),"%Y/%m/%d %H:%M:%S",timeinfo);
	else
		strftime (buffers,sizeof(buffers),format,timeinfo);
	
	//sprintf(buffer,"%s",buffers);
	strcpy(buffer,buffers);
}

char* createGUID(char* uid){
	
#ifdef WIN32
	char buffer[GUID_LEN] = { 0 };
    GUID guid;
    if(CoCreateGuid(&guid))
    {
    	//printf(stderr, "create guid error\n");
    	return NULL;
    }
    _snprintf(buffer, sizeof(buffer),
    "%08X%04X%04x%02X%02X%02X%02X%02X%02X%02X%02X",
    guid.Data1, guid.Data2, guid.Data3,
    guid.Data4[0], guid.Data4[1], guid.Data4[2],
    guid.Data4[3], guid.Data4[4], guid.Data4[5],
    guid.Data4[6], guid.Data4[7]);
#else

	if(uid == NULL) return NULL;
	bzero(uid,32);
	uuid_t uu;
    uuid_generate(uu);
    //printf("{");
	int i = 0;
    for(i=0;i<15;i++)
        sprintf(uid,"%s%02X",uid,uu[i]);
	return uid;
	//TODO
    //printf("%02X}\n",uu[15]);
#endif	

}
int splitcountx(char* str, const char* spl)
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
	if(dst == NULL){
		result = strtok(str, spl);	
		while( result != NULL )
		{
			//strcpy(dst[n++], result);
			n++;
			result = strtok(NULL, spl);
		}
		dst = malloc(n);
		int i =0;
		for(i=0;i<n;i++){
			dst[i] = malloc(64);
		}
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

char* getFileName(char* fullpath){
	int len;
    char *current=NULL;

    len=strlen(fullpath);   
    for (;len>0;len--) //从最后一个元素开始找.直到找到第一个'/'
    {    	
        if(fullpath[len]=='/')
        {
            current=&fullpath[len+1];
            return current;
        }
    }
    //for windows file
    if(current == NULL){
    	for (;len>0;len--) //从最后一个元素开始找.直到找到第一个'/'
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
char* createfullpath(const char* filePath,const char* fileName,char* fullpath){
	//char fullpath[500]= {0};
    int length = strlen(filePath);
    if(length != 0 && filePath != NULL && filePath[0] != 0){    	
	    if(filePath[length] == '\\' || filePath[length] == '/'){
	    	sprintf(fullpath,"%s%s%s",fullpath,filePath,fileName);
	    }else{
			sprintf(fullpath,"%s%s/%s",fullpath,filePath,fileName);
	    }
	}
	else{
		sprintf(fullpath,"%s",fileName);
	}
	return fullpath;
}
int writetofile(const char* filePath,const char* fileName,void* content,int len)
{
    char fullpath[500]= {0};
    int length = strlen(filePath);
    if(length != 0 && filePath != NULL && filePath[0] != 0){    	
	    if(filePath[length] == '\\' || filePath[length] == '/'){
	    	sprintf(fullpath,"%s%s%s",fullpath,filePath,fileName);
	    }else{
			sprintf(fullpath,"%s%s/%s",fullpath,filePath,fileName);
	    }
	}
	else{
		sprintf(fullpath,"%s",fileName);
	}

    FILE * flogx=fopen(fullpath,"a");
    int ret = -1;
    if (NULL!=flogx)
    {
        //fprintf(flog,"%s %s.%s File:%s(%d) %s",datestr,timestr,ms10,file,line,logstr);
        if(len< FILE_WRITE_BUFF_LEN)
        {
            ret = fwrite(content,len,1,flogx);
        }
        else
        {
            int sum = 0;
            while(1)
            {	
            	//printf("%d\n",len-sum);
                if((len-sum)>FILE_WRITE_BUFF_LEN)
                {
                    ret = fwrite(content+sum,sizeof( unsigned char ),FILE_WRITE_BUFF_LEN,flogx);
                }
                else
                {
                    ret = fwrite(content+sum,sizeof( unsigned char ),len-sum,flogx);
                    sum+=ret;
                    ret = sum;
                    break;
                }
                sum+=ret;
            }
        }
        fclose(flogx);
    }
    return ret;

}
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
    if(length != 0 && filePath != NULL && filePath[0] != 0){    		
	    if(filePath[length] == '\\' || filePath[length] == '/'){
	    	sprintf(fullpath,"%s%s%s",fullpath,filePath,fileName);
	    }else{
			sprintf(fullpath,"%s%s/%s",fullpath,filePath,fileName);
	    }
	}
	else{
		sprintf(fullpath,"%s",fileName);
	}

    long len = get_file_size(fullpath);
    printf("read file:%s:%d\n",fullpath,len);
    FILE * flogx=fopen(fullpath,"rb");
    int sum = 0;
    void* buf = malloc(len);
    if (NULL!=flogx)
    {
        *ret = fread(buf,1,len,flogx);
        fclose(flogx);
    }
    else
    {
        printf("read file filed!\n");
    }
    return buf;
}
void itoa_(int i,char*string)
{
    int power,j;
    j=i;
    for(power=1; j>=10; j/=10)
        power*=10;
    for(; power>0; power/=10)
    {
        *string++='0'+i/power;
        i%=power;
    }
    *string='\0';
}
/*
int main(int argc,char * argv[]) {
    int i;
#ifdef WIN32
    InitializeCriticalSection(&cs_log);
#else
    pthread_mutex_init(&cs_log,NULL);
#endif
    for (i=0;i<10000;i++) {
        Log("This is a Log %04d from FILE:%s LINE:%d\n",i, __FILE__, __LINE__);
    }
#ifdef WIN32
    DeleteCriticalSection(&cs_log);
#else
    pthread_mutex_destroy(&cs_log);
#endif
    return 0;
}
*/
