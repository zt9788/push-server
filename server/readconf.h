#ifndef READCONF_H
#define READCONF_H
#ifdef WIN32
#include <windows.h>
#endif
#define MAXBUF 4096

#ifndef NULL
#define NULL 0
#endif


/*   删除左边的空格   */
char * l_trim(char * szOutput, const char *szInput);
/*   删除右边的空格   */
char *r_trim(char *szOutput, const char *szInput);

/*   删除两边的空格   */
char * a_trim(char * szOutput, const char * szInput);


int _GetProfileString(char *profile, char *AppName, char *KeyName, char *KeyVal );

#endif /* !READCONF_H */