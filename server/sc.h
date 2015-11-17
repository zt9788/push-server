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
#ifndef SC_H
#define SC_H
#include <netinet/in.h>

//message content type
/*
#define MESSAGE_TYPE_TXT  0xF11
#define MESSAGE_TYPE_IMG  0xF22
#define MESSAGE_TYPE_AUDIO  0xF22
#define MESSAGE_TYPE_VIDEO  0xF22
#define MESSAGE_TYPE_FILE  0xF22

#define MESSAGE_TYPE_LONG_MESSAGE 0xF30
#define MESSAGE_TYPE_GET_MESSAGE_LIST 0xF31
#define MESSAGE_TYPE_GET_MESSAGE 0xF41
*/
#define MESSAGE_TYPE_TXT  0xF1
#define MESSAGE_TYPE_IMG  0xF2
#define MESSAGE_TYPE_AUDIO  0xF3
#define MESSAGE_TYPE_VIDEO  0xF4
#define MESSAGE_TYPE_FILE  0xF5

#define MESSAGE_TYPE_LONG_MESSAGE 0xF6
#define MESSAGE_TYPE_GET_MESSAGE_LIST 0xF7
#define MESSAGE_TYPE_GET_MESSAGE 0xF8

//message type
#define MESSAGE_HELO 0xA0
#define MESSAGE_PING 0xA1
#define MESSAGE 	 0xA2
#define MESSAGE_SERVER 0xA3
#define MESSAGE_BYE 0xA4
#define MESSAGE_YES 0xA5

#define COMMAND_HELO 		0xA0
#define COMMAND_PING 		0xA1
#define COMMAND_MESSAGE 	0xA2
#define COMMAND_SERVER 		0xA3
#define COMMAND_BYE 		0xA4
#define COMMAND_YES 		0xA5

#ifdef S_OK
	#undef S_OK
#endif
#define S_OK		0xA6
//response type
#define MESSAGE_SUCCESS 0x01
#define MESSAGE_FILDE 0x00

//communication protocol
#define PROTOCOL_SELF  			0x0A 
#define PROTOCOL_SSL_SELF  		0x0B
#define PROTOCOL_WEBSOCKET 		0x1A
#define PROTOCOL_SSL_WEBSOCKET  0x1B
#define PROTOCOL_XMPP      		0x0C
#define PROTOCOL_OHTER 			0x0D//


//clienttype

#define CLIENT_TYPE_IOS 0x1A
#define CLIENT_TYPE_ANDROID 0x2A
#define CLIENT_TYPE_PC 0x3A
#define CLIENT_TYPE_WEB 0x4A

#ifndef BYTE
typedef  unsigned char BYTE;
#endif
/***********************************************

     byte      byte         byte                           =====> all header ,ping,or others   < 4096    
   command   clienttype  messagetype  
   
0)   byte[2]       char[~64]                               =====> in helo command   < 4096   
0) fromlength    fromdrivceid    

1)  byte[2]    char[~64]    byte[2]          bytes         =====> in helo command   < 4096   
1) idlength   messageid    contentlength    content     

2)  byte[2]    char[~64]  byte[2]  char[255]   long         bytes         =====> in helo command   < 4096   
2) idlength   messageid   fnlength filename  contentlength    content     

********************************************/

typedef struct server_header_2{
	unsigned char command;//command 	
	unsigned char messagetype;//
	short serverid;
	int total;
}server_header_2_t;


/***********************************************

     byte      byte         byte                         			=====> all header ,ping,or others   < 4096    
   command   clienttype  messagetype  
   
0)   byte[2]        char[~64]    byte[2]        char[~64]           =====> in helo command   < 4096   
0) drivceidlength  drivceid     iostoenlength   iostoken
   
1)   short       byte[2]     byte[][2]   char[][~64]     			=====> in message command  < 4096   
1)  delytime  sendtocount     tolength       to  

2)  byte[2]         bytes                                			=====> in text message  < 4096
2) contentlength   content

3)  byte[2]         bytes      long         bytes        			=====> in file message
3) filenamelength  filename	contentlength  content	

4)  long         bytes                                   			=====> in long message
4) length      content

5)  byte[2]      bytes                                   			=====> in get message by messageid
5) idlength    messageid

6)  byte[2]       byte[2]                                			=====> in get message list(sort count)
6) message-star  message-end
********************************************/
typedef struct client_header_2{
	unsigned char command;//command
	unsigned char clienttype;
	unsigned char messagetype;//
	int total;
}client_header_2_t;


typedef struct server_config_to_client{
	BYTE isOpenMessageResponse;
	BYTE isOpenFileSocket;
	BYTE isOpenPingResponse;	
}server_config_to_client_t;

typedef struct CLIENT_HEADER{
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
}CLIENT_HEADER;



typedef struct SEND_TO{
    char ip[16];
    char sendtoken[64];
    int userid;
}SEND_TO;

typedef struct SERVER_HEADER{
    int messageCode;
	int type;
	int messagetype;
	int contentLength;
	int totalLength;
	char fromtoken[64];  
}SERVER_HEADER;


typedef struct CLIENT {
    int fd;
    int clientId;
    int serverid;
	char drivceId[64];	
	int protocol;
	int clienttype;/*0=android,1=ios,2=web*/
    struct sockaddr_in addr;
    pthread_mutex_t opt_lock;
}CLIENT;



#define MAX_CONNECTION_LENGTH 102400
#define MAX_EVENTS 500  
#define REVLEN 1024

#endif /* !SC_H */