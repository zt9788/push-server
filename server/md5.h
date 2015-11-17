#ifndef MD5_H
#define MD5_H

#ifdef __alpha
typedef unsigned int uint32;
#else
//#ifndef _WIN32_ 
typedef unsigned long uint32;
//#endif
#endif

struct MD5Context {
        uint32 buf[4];
        uint32 bits[2];
        unsigned char in[64];
}MD5Context;

extern void MD5Init();
extern void MD5Update();
extern void MD5Final();
extern void MD5Transform();

extern char* createMd5(const char* str,char* retStr);
/*
* This is needed to make RSAREF happy on some MS-DOS compilers.
*/
typedef struct MD5Context MD5_CTX;

#endif /* !MD5_H */