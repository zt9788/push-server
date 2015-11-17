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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/syslog.h>
#include "daemon.h"
#include<sys/wait.h>
#include <string.h>
#ifdef DEBUG
#include "leak_detector_c.h"
#endif 

extern  volatile int g_isExit ;
extern int g_isRestart;
extern int main_pid;
#define NOFILE 65535
#define PATH_MAX 500

#define FILEPATH_MAX 500
char* getRealPath(char* file_path_getcwd){
	//char *file_path_getcwd;
    //file_path_getcwd=(char *)malloc(FILEPATH_MAX);
    getcwd(file_path_getcwd,PATH_MAX);
//    printf("%s",file_path_getcwd);
    return file_path_getcwd;
	
}

int readPidFile(const char* szPidFile){
	
	char  str[32];
    int lfp = open(szPidFile, O_RDONLY,  S_IRUSR | S_IWUSR);
    if (lfp < 0) return -1;
    char buf[255];
    int bytes_read = read(lfp,buf,255);
    
 	if ((bytes_read == -1) && (errno != EINTR)) {
 		close(lfp);
	 	return -1;
 	}
	int pid = atoi(buf);    
    close(lfp);
    return pid;
}
int deletePidFile(const char *szPidFile){
	
	int lfp = open(szPidFile, O_RDONLY,  S_IRUSR | S_IWUSR);
	if (lfp < 0) return -1;
	if (lockf(lfp, F_ULOCK, 0) < 0) {
		fprintf(stderr, "Can't Open Pid File: %s", szPidFile);
        close(lfp);
        exit(0);
    }
    close(lfp);
    remove(szPidFile);
}
void writePidFile(const char *szPidFile)
{
    char            str[32];
    int lfp = open(szPidFile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (lfp < 0) exit(1);

// lockf - apply, test or remove a POSIX lock on an open file
// cmd : F_LOCK(block&lock) F_TLOCK(try&lock) F_ULOCK(unlock) F_TEST(will not lock)
// the call never blocks and returns an error
    // instead if the file is already locked.

// lock will release when close the file descriptor if lock sucess.
    if (lockf(lfp, F_TLOCK, 0) < 0) {
        fprintf(stderr, "Can't Open Pid File: %s", szPidFile);
        close(lfp);
        exit(0);
    }

// get the pid,and write it to the pid file.
    sprintf(str, "%d\n", getpid()); // \n is a symbol.
    int len = strlen(str);
    ssize_t ret = write(lfp, str, len);
    if (ret != len ) {
        fprintf(stderr, "Can't Write Pid File: %s", szPidFile);
        close(lfp);
        exit(0);
    }
    close(lfp);
}
void closeServer(int pid ,int sig){
	sigval_t sigval;   
    sigval.sival_int = 0x12345678;
	kill(pid,SIGTERM);//sig
}
void restartServer(int pid ,int sig){
	sigval_t sigval;   
    sigval.sival_int = 0x12345678;
	kill(pid,SIGHUP);//sig
}
void wait_close(int sig){
	g_isExit = 1;	
	(void) signal(SIGTERM, SIG_DFL);  
}
void restart_server(int sig){
	g_isExit = 1;
	g_isRestart = 1;
	(void) signal(SIGHUP, SIG_DFL);  
}
void restart(char *fileName){
	int ret = system(fileName);	
	 if (WIFEXITED(ret))  
        {  
            if (0 == WEXITSTATUS(ret))  {
            	
            }
            else{
            	 printf("run program fail, exit code: %d\n", WEXITSTATUS(ret));              	
            }
        }
	
}
//SIGPIPE是正常的信号，对端断开后本端协议栈在向对端传送数据时，对端会返回TCP RST，导致本端抛出SIGPIPE信号
void init_daemon(char* cmd) 
{ 
	int pid;  
    char *buf = "This is a Daemon, wcdj\n";  
    char path[FILEPATH_MAX];
    getRealPath(path);
    strcat(path,"/");
	strcat(path,"pushserver.lock");
	pid = readPidFile(path);
	if(pid != -1)
	{
    	printf("The server is start ...\n");    	
    }    	    
    if(strcmp(cmd,"stop") == 0 && pid != -1){
    	closeServer(pid,0);
    	int status;
    	int w;
    	 do {
            w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            if (w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

            if (WIFEXITED(status)) {
                printf("exited, status=%d/n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d/n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d/n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("continued/n");
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    	exit(0);
    }    
    else if(strcmp(cmd,"restart") == 0 && pid != -1){
    	int status;
    	int w;
    	 do {
            w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
            if (w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

            if (WIFEXITED(status)) {
                printf("exited, status=%d/n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("killed by signal %d/n", WTERMSIG(status));
            } else if (WIFSTOPPED(status)) {
                printf("stopped by signal %d/n", WSTOPSIG(status));
            } else if (WIFCONTINUED(status)) {
                printf("continued/n");
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    	
    }else if(pid != -1){
    	printf("The server is start ...\n");   
    	exit(0);
    }
  	
    /* 屏蔽一些有关控制终端操作的信号 
     * 防止在守护进程没有正常运转起来时，因控制终端受到干扰退出或挂起 
     */  
    signal(SIGINT,  SIG_IGN);// 终端中断  
    signal(SIGHUP,  SIG_IGN);// 连接挂断  
    signal(SIGQUIT, SIG_IGN);// 终端退出  
    signal(SIGPIPE, SIG_IGN);// 向无读进程的管道写数据  
    signal(SIGTTOU, SIG_IGN);// 后台程序尝试写操作  
    signal(SIGTTIN, SIG_IGN);// 后台程序尝试读操作  
    signal(SIGTERM, SIG_IGN);// 终止  
    
    struct rlimit        rl;
 	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        printf("%s: can't get file limit", "pushserver");
        
	int i; 
	/*
	pid=fork();
	if(pid>0) 
		exit(0);//是父进程，结束父进程 
	else if(pid< 0) 
		exit(1);//fork失败，退出 
	//是第一子进程，后台继续执行 
	setsid();//第一子进程成为新的会话组长和进程组长 
	//并与控制终端分离 
	pid=fork();
	if(pid>0) 
		exit(0);//是第一子进程，结束第一子进程 
	else if(pid< 0) 
		exit(1);//fork失败，退出 
		*/
	if ((pid = fork()) < 0)
        printf("%s: can't fork", cmd);
    else if (pid != 0) /* parent */{
    	printf("tuichuzhu\n");
    	   exit(0);	
    }
    printf("jinlaile\n");
     
        
    setsid();
         
	//是第二子进程，继续 
	//第二子进程不再是会话组长 
	
	main_pid = getpid();     
	writePidFile(path);
	     
	for(i=3;i< NOFILE;++i)//关闭打开的文件描述符 MAXFILE 65536
		close(i); 
		     
	/*
	// [3] set current path  
    char szPath[1024];  
    if(getcwd(szPath, sizeof(szPath)) == NULL)  
    {  
        perror("getcwd");  
        exit(1);  
    }  
    else  
    {  
        chdir(szPath);  
        printf("set current path succ [%s]\n", szPath);  
    }  
    */     
	//chdir("/tmp");//改变工作目录到/tmp 
	umask(0);//重设文件创建掩模 
	 // [6] set termianl signal  
	     
 	signal(SIGTERM, wait_close); 
	signal(SIGHUP, restart_server);
	     
	/*
     * Close all open file descriptors.
     */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);
     
    /*
     * Attach file descriptors 0, 1, and 2 to /dev/null.
     */     
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);
     
    /*
     * Initialize the log file.
     */     
    openlog("pushserver", LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {     
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
        exit(1);
    }     
    syslog(LOG_DEBUG, "daem ok ");
           
	return; 
} 
/**************************************
 *
 *
 *
 ***********************************/
config_struct* read_config(char* file,struct config_struct* _config)
{
    if(_config == NULL) return NULL;
    //[server]
    //serverid
    char tempStr[50] = {0};
    if(_GetProfileString(file, "server", "serverid", tempStr)!=0)
        _config->serverid = 1;
    else
        _config->serverid = atoi(tempStr);
    
    //server_port
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "port", tempStr)!=0)
        _config->server_port = 1234;
    else
        _config->server_port = atoi(tempStr);

    //push_time_out
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "push_time_out", tempStr)!=0)
        _config->push_time_out = 1;
    else
        _config->push_time_out = atoi(tempStr);
        
        
    //unin_thread_count
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "unin_thread_count", tempStr)!=0)
        _config->unin_thread_count = 1;
    else
        _config->unin_thread_count = atoi(tempStr);

    //delay_time_out
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "delay_time_out", tempStr)!=0)
        _config->delay_time_out = 20;
    else
        _config->delay_time_out = atoi(tempStr);

    //delay_time_out
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "push_thread_count", tempStr)!=0)
        _config->push_thread_count = 2;
    else
        _config->push_thread_count = atoi(tempStr);

    //push_time_out
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "push_time_out", tempStr)!=0)
        _config->push_time_out = 5;
    else
        _config->push_time_out = atoi(tempStr);

    //processer
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "processer", tempStr)!=0)
        _config->processer = 0;
    else
        _config->processer = atoi(tempStr)-1;

    //max_send_thread_pool
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "max_send_thread_pool", tempStr)!=0)
        _config->max_send_thread_pool = 5;
    else
        _config->max_send_thread_pool = atoi(tempStr);

    //max_recv_thread_pool
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "max_recv_thread_pool", tempStr)!=0)
        _config->max_recv_thread_pool = 100;
    else
        _config->max_recv_thread_pool = atoi(tempStr);

    //[redis]
    //redis ip
    bzero(tempStr,50);
    if(_GetProfileString(file, "redis", "ip", tempStr)!=0)
        strcpy(_config->redis_ip,"127.0.0.1");
    else
        strcpy(_config->redis_ip,tempStr);

    //redis port
    bzero(tempStr,50);
    if(_GetProfileString(file, "redis", "port", tempStr)!=0)
        _config->redis_port = 6379;
    else
        _config->redis_port = atoi(tempStr);

    //deamon
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "deamon", tempStr)!=0)
        strcpy(_config->deamon,"no");
    else
        strcpy(_config->deamon,"tempStr");

    //deamon
    bzero(tempStr,50);
    if(_GetProfileString(file, "server", "tempPath", tempStr)!=0)
        strcpy(_config->tempPath,"");
    else
        strcpy(_config->tempPath,"tempStr");

    //max_redis_pool
    bzero(tempStr,50);
    if(_GetProfileString(file, "redis", "max_redis_pool", tempStr)!=0)
        _config->max_redis_pool = 20;
    else
        _config->max_redis_pool = atoi(tempStr);
}

void print_welcome(){
	srand((unsigned) time(NULL)); 
	int number = rand() % 3; 
	if(number == 1){			
		printf( "+-----------------------------------------------+\n");
		printf( "+-----------------------------------------------+\n");
		printf( "+                   _ooOoo_                     +\n");
		printf( "+                  o8888888o                    +\n");
		printf( "+                  88\" . \"88                    +\n");
		printf( "+                  (| -_- |)                    +\n");
		printf( "+                  O\\  =  /O                    +\n");         
		printf( "+               ____/`---'\\____                 +\n");
		printf( "+             .'  \\\\|     |//  `.               +\n");
		printf( "+            /  \\\\|||  :  |||//  \\              +\n");
		printf( "+           /  _||||| -:- |||||-  \\             +\n");
		printf( "+           |   | \\\\\\  -  /// |   |             +\n"); 
		printf( "+           | \\_|  ''\\---/''  |   |             +\n");
		printf( "+           \\  .-\\__  `-`  ___/-. /             +\n");
		printf( "+         ___`. .'  /--.--\\  `. . __            +\n");
		printf( "+      .\"\" '<  `.___\\_<|>_/___.'  >'\"\".         +\n");  
		printf( "+     | | :  `- \\`.;`\\ _ /`;.`/ - ` : | |       +\n");        
		printf( "+     \\  \\ `-.   \\_ __\\ /__ _/   .-` /  /       +\n");      
		printf( "+======`-.____`-.___\\_____/___.-`____.-'======  +\n");              
		printf( "+                   `=---='                     +\n");          
		printf( "+-----------------------------------------------+\n");
	 	printf( "+-----------------------------------------------+\n");
	}
	else{
		print_welcome2();
	}
}
void print_welcome2(){
	printf( "                    .....      .....          \n");   
	printf( "                ...  ..........   .....       \n");  
	printf( "             .......              .. .....       \n");  
	printf( "            .....i,                ..i, ....     \n");  
	printf( "          101.. . .ifi..         :tt:  . .:CC.   \n");  
	printf( "         f@@@@C.. .  .it.      it,      i8@@@0    \n"); 
	printf( "        :8@@@@@G.  .i  ,1.   .;;  ;,   1@@@@@8f   \n");  
	printf( "        .  .f@@@f ...G .,;   .;  11.  ,@@@G,       \n"); 
	printf( "            . L@0    Lt  i   ;. ,0. . 1@8,     .   \n");  
	printf( "       .tL:    ;@t  .Lt, i.  1. fii  ,@L    ,tL:   \n");  
	printf( "       ..;@@0;  .L@@G.11 t,. 1,.G t@88:  .L@@L     \n");  
	printf( "      ;1, C@@@8;     ,8L.t.i.i;:@t     .C@@@@..11  \n");  
	printf( "      LLi i@@@@@8GtC8@@L.t:;ii;:@@8GtL08@@@@C ,LL: \n");  
	printf( "      Lf..,@@@@@@CiLL;Ci :;.,;..0;LL1t8@@@@@t .fL: \n");  
	printf( "      i;i; L@@@@@@@@@@C.,.    : ;@@@@@@@@@@8.,1ii  \n");  
	printf( "       it:. L@@@@@@@8i 1:     .f.,G@@@@@@@8, .f1.   \n");  
	printf( "       :, .   :tf1:. .Ci1     ,iG:  ,itti..... i  \n"); 
	printf( "        1;;.  .    .,8iL,t; .t:f,8t    ... .,ii,  \n");  
	printf( "          ... . .  t@t    t@0.  .,@G..   ....     \n");   
	printf( "           ...  ..0@@1  ;0@@@@f. .8@8i   ....      \n");   
	printf( "           ..  .,8@@@@@@@@@@@@@@@@@@@@1. ...      \n");    
	printf( "            .. .f@@@8;,;t;;1i;11,,C@@@8.. .       \n");     
	printf( "             . .f@@@;,.,,,:;;:,,,,.0@@8. .      \n");       
	printf( "               ..G@@i,,,,,,,,,,,,,,0@@;..    \n");          
	printf( "                ..f@8:,.,,,,,,,,..L@0,.    \n");          
	printf( "                   .f8t.,,,,,,,,:0G:     \n");              
	printf( "                       .,,,,,,,,        \n");     
}