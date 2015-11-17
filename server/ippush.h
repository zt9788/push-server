#ifndef IPPUSH_H
#define IPPUSH_H

#define MAX_PAYLOAD_SIZE 256  
#define TOKEN_SIZE 32  

typedef struct push_message{
	long long id;
	char token[64];
	char message[2048];
}push_message_t;

int pushMessage_2(char* host,
				int port,
				const char* clientcert,
	 			const char* clientkey, /* 客户端的Key */  
        		const char* keypwd, /* 客户端Key的密码, 如果有的话 */  
        		const char* cacert, /* 服务器CA证书 如果有的话 */  
        		const char* token,const char* message);

int pushMessage(char* host,
				int port,
				const char* clientcert,
	 			const char* clientkey, /* 客户端的Key */  
        		const char* keypwd, /* 客户端Key的密码, 如果有的话 */  
        		const char* cacert, /* 服务器CA证书 如果有的话 */  
        		//int badage, const char *sound,
				push_message_t*(*getMessageInfo)(void* args),void* args,
				void(*sendMssageOK)(int result,long long job));
				
				
				
///////////////////////////////////////////////////////////////
// next is privte method
////////////////////////////////////////////////////////////

void DeviceToken2Binary(const char* sz, const int len, unsigned char* const binary, const int size) ;
void DeviceBinary2Token(const unsigned char* data, const int len, char* const token, const int size) ;
void Closesocket(int socket) ;
void init_openssl() ;
SSL_CTX* init_ssl_context(  
        const char* clientcert, /* 客户端的证书 */  
        const char* clientkey, /* 客户端的Key */  
        const char* keypwd, /* 客户端Key的密码, 如果有的话 */  
        const char* cacert); /* 服务器CA证书 如果有的话 */  
int tcp_connect(const char* host, int port) ;
SSL* ssl_connect(SSL_CTX* ctx, int socket) ;
int verify_connection(SSL* ssl, const char* peername);
void json_escape(char*  str);
int build_payload(char* buffer, int* plen, char* msg, int badage, const char * sound);
int build_output_packet(char* buf, int buflen, /* 输出的缓冲区及长度 */  
        const char* tokenbinary, /* 二进制的Token */  
        char* msg, /* 要发送的消息 */  
        int badage, /* 应用图标上显示的数字 */  
        const char * sound); /* 设备收到时播放的声音，可以为空 */         
int send_message(SSL *ssl, const char* token, char* msg, int badage, const char* sound)  ;
int build_output_packet_2(char* buf, int buflen, /* 缓冲区及长度 */  
        uint32_t messageid, /* 消息编号 */  
        uint32_t expiry, /* 过期时间 */  
        const char* tokenbinary, /* 二进制Token */  
        char* msg, /* message */  
        int badage, /* badage */  
        const char * sound) ;/* sound */          
int send_message_2(SSL *ssl, const char* token, uint32_t id, uint32_t expire, char* msg, int badage, const char* sound);
int send_message_3(SSL *ssl, const char* token, char* msg);
char *trim(char *str,char trimchar);

#endif /* !IPPUSH_H */