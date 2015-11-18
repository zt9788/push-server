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
#ifndef DAEMON_H
#define DAEMON_H

typedef struct config_struct
{
	int serverid;
	int server_port;
	int push_time_out;
	int delay_time_out ;
	int push_thread_count;
	int unin_thread_count;
	int processer;
	int max_send_thread_pool;
	int max_recv_thread_pool;
	int max_redis_pool;
	char redis_ip[20];
	char deamon[20];
	int redis_port;
	char tempPath[500];
	int socket_time_out;//default 120
}config_struct;

void init_daemon(char* cmd);
void closeServer(int pid ,int sig);
void restartServer(int pid ,int sig);
char* getRealPath(char* file_path_getcwd);
void writePidFile(const char *szPidFile);
int readPidFile(const char* szPidFile);
void restart(char *fileName);
int deletePidFile(const char* szPidFile);
config_struct* read_config(char* file,struct config_struct* _config);
void print_welcome();
void print_welcome2();

#endif /* !DAEMON_H */