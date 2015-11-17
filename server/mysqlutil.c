

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>  
#include <unistd.h>  
#include <fcntl.h>  
#include <errno.h>
#include <stdlib.h>  
#include <stdint.h>  
#include <assert.h>  
#include <mysql/mysql.h>
#include "mysqlutil.h"
#include "log.h"
#define __DEBUG(format, ...) printf("FILE: "__FILE__", LINE: %d: "format"/n", __LINE__, ##__VA_ARGS__)
//static 
typedef struct mysql_pool_work{
	MYSQL* connect ;
	char serverip[20];
	int port;
	char dbuser[50];
	char pass[255];
	char dbname[255];
	int masterOrSalve;	
	int connectid;
	int attri;
	int count;
	pthread_mutex_t mysql_opt_lock;
	pthread_mutex_t mysql_get_lock;
	struct mysql_pool_work* next;
}mysql_pool_work_t;



static mysql_pool_work_t* queue_head = NULL;
static mysql_pool_work_t* g_config_default = NULL;
static int max_mysql_pool_size = 0;
static int mysql_pool_size = 0;
static int connectid = 0;

pthread_mutex_t mysql_pook_lock;




MYSQL* mysql_create(char* serverip,
					char* dbuser,
					char* pass,
					char* dbname, 
					int port,
					int attri,
					int masterOrSalve);
void closeConnect(int fd);					
				
void init(char* serverip,
					char* dbuser,
					char* pass,
					char* dbname, 
					int port,
					int attri,
					int masterOrSalve){
	sprintf(g_config_default->serverip,"%s",serverip);
	sprintf(g_config_default->dbuser,"%s",dbuser);
	sprintf(g_config_default->pass,"%s",pass);
	sprintf(g_config_default->dbname,"%s",dbname);
	g_config_default->port = port;
	g_config_default->attri = attri;
	g_config_default->masterOrSalve = masterOrSalve;
	
	
}					
mysql_pool_work_t* getConn(int fd){
	
	mysql_pool_work_t* work = queue_head;
	mysql_pool_work_t* result = NULL;
	mysql_pool_work_t* prework = NULL;
	while(work){
		if(work->connectid == fd)
		{
			result = work;
			break;
		}
		prework = work;
		work = work->next;
	}
	return result;
}
void destroyMysqlPool(){
	mysql_pool_work_t* work = queue_head;
	mysql_pool_work_t* temp = NULL;
	while(work){
		temp = work;
		pthread_mutex_destroy(&temp->mysql_opt_lock); 
		pthread_mutex_destroy(&temp->mysql_get_lock);
		//TODO has error
		closeConnect(temp->connectid); 
		work = work->next;
		free(temp);
	}
	mysql_pool_size = 0;
	connectid = 0;
	free(g_config_default);
}
//the Method still need to change linklist 
//this Method donot use it!!!!!
void closeConnect(int fd)
{
	mysql_pool_work_t* work = queue_head;
	mysql_pool_work_t* result = NULL;	
	mysql_pool_work_t* prework = NULL;
	while(work){
		if(work->connectid == fd)
		{
			result = work;
			break;
		}
		prework = work;
		work = work->next;
	}
		
	if(result != NULL){
		pthread_mutex_unlock(&result->mysql_opt_lock);
		pthread_mutex_unlock(&work->mysql_opt_lock);
		pthread_mutex_lock(&result->mysql_get_lock);
		pthread_mutex_lock(&work->mysql_opt_lock);
		if ( 0 == mysql_ping(result->connect) )
		{
			mysql_close(result->connect);
		}
		pthread_mutex_unlock(&work->mysql_opt_lock);
		pthread_mutex_unlock(&result->mysql_get_lock);
		//pthread_mutex_destroy(&result->mysql_get_lock);
		if(work->next && prework != NULL){
			prework->next = result->next;	
		}else{
			prework->next = NULL;
		}		
		free(result);
		//mysql_pool_size--;
		//TODO here need to change linklist so this function donot use it!!!!!
	}	
	//
}
int createMysqlPool(int max_pool){
	max_mysql_pool_size = max_pool;	
	//pthread_mutex_init(&mysql_get_lock,NULL);
	pthread_mutex_init(&mysql_pook_lock,NULL);
	g_config_default = malloc(sizeof(mysql_pool_work_t));
}

int commit(int fd)
{
	mysql_pool_work_t* work = getConn(fd);
	pthread_mutex_lock(&work->mysql_opt_lock);
	my_bool res = mysql_commit(work->connect);
	pthread_mutex_unlock(&work->mysql_opt_lock);
	return res;
}
int queryResult(int fd,mysql_row_result_t** result,char* sqlformat,...){
	mysql_pool_work_t* work = getConn(fd);	
	va_list arglist;
	va_start( arglist, sqlformat );
	pthread_mutex_lock(&work->mysql_opt_lock);
	int row_num = _query(fd,result,sqlformat,arglist);
	pthread_mutex_unlock(&work->mysql_opt_lock);
	va_end( arglist );
	return row_num;
}
void freeArray(char** src,int len){
	int i=0;
	for(i=0;i<len;i++){
		if(src[i] != NULL)
			free(src[i]);		
	}
	free(src);
}
char** getContent2Array(int row,const mysql_row_result_t* result,char** des){
	mysql_row_result_t* head = result;//columncount	
	while(head){
		if(des == NULL){
			des = (char**)malloc(head->columncount*sizeof(char *));
		}
		if(head->rowid == row){					
			mysql_column_result_t* column = head->columns;
			int i = 0;		
			while(column){
				if(column->content != NULL){
					des[i] = malloc(sizeof(column->content)+1);
				}
				des[i] = column->content;
				i++;	//char* content = malloc()
				column = column->next;
			}
		}
		head = head->next;
	}	
	return des;
}
char* getContentByName(int row,char* columnname,const mysql_row_result_t* result){
	mysql_row_result_t* head = result;
	while(head){
		printf("rowid = %d\n",head->rowid);
		if(head->rowid == row){					
			mysql_column_result_t* column = head->columns;		
			while(column){
				printf("%s==%s\n",column->columnname,columnname);
				if(strcasecmp(column->columnname,columnname) == 0 )
					return column->content;
				column = column->next;
			}
		}
		head = head->next;
	}	
}
char* getContentById(int row,int columnid,const mysql_row_result_t* result){
	mysql_row_result_t* head = result;
	while(head){
		if(head->rowid == row){					
			mysql_column_result_t* column = head->columns;		
			while(column){
				if(column->columnid == columnid)
					return column->content;
				column = column->next;
			}
		}
		head = head->next;
	}	
}
void freeResult(mysql_row_result_t* result){
	mysql_row_result_t* head = result;
	while(head){
		mysql_column_result_t* column = head->columns;		
		while(column){
			if(column->content != NULL)
				free(column->content);
			mysql_column_result_t* tempcol = column;	
			column = column->next;
			free(tempcol);
		}
		mysql_column_result_t* temprow = head;
		head = head->next;
		free(temprow);
	}	
}
void returnMysql(int fd){
	mysql_pool_work_t* work = getConn(fd);
	if(work != NULL)
		pthread_mutex_unlock(&work->mysql_get_lock);
}

int _query(int fd,mysql_row_result_t** result,char* sql,va_list arglist){
	mysql_pool_work_t* work = getConn(fd);
	int i=0,j = 0;
	if( _executeEx(fd,sql,arglist) != 0){
		printf("%s%d _executeEx error\n",__FILE__,__LINE__);
		return -1;			
	}		
		
	MYSQL_RES *m_mysql_res = mysql_store_result( work->connect );
	
	if ( m_mysql_res )
	{
		MYSQL_FIELD *m_fields = mysql_fetch_fields( m_mysql_res);
		unsigned int m_numbers_of_fields = mysql_num_fields( m_mysql_res );
		
		printf("m_numbers_of_fields:%d\n",m_numbers_of_fields);
		MYSQL_ROW my_row = NULL;			
		
	     while(my_row=mysql_fetch_row(m_mysql_res)){
			unsigned long *lengths = mysql_fetch_lengths(m_mysql_res);
            mysql_row_result_t* row = malloc(sizeof(mysql_row_result_t));
            row->next = NULL;
            mysql_column_result_t* column_header = NULL;	
         	        	
	        for(i=0; i<m_numbers_of_fields; i++)  
	        {  
		        mysql_column_result_t* column = malloc(sizeof(mysql_column_result_t)); 
		        column->next = NULL;
	        	column->columnid = i;
	        	strcpy(column->columnname,m_fields[i].name);
	            if( my_row[i] == NULL ) {
            		 column->content = NULL;
            	} 	              
	            else{
					//char* content = malloc(lengths[i]);//strlen(my_row[i])+1);
					printf("%d,%s,%d\n",i,my_row[i],strlen(my_row[i]));
					dump_data(my_row[i],strlen(my_row[i])); 
					char* content = malloc(strlen(my_row[i])+1);
					//bzero(content,lengths[i]);
					bzero(content,strlen(my_row[i])+1);
					//memcpy(content,my_row[i],lengths[i]);
					strcpy(content,my_row[i]);
					column->content = content;	
            	} 
            	mysql_column_result_t* column_temp = column_header;
				if(column_header == NULL) 
					column_header = column;
				else{
					while(column_temp->next)
			        {
			            column_temp = column_temp->next;
			        }
			        column_temp->next = column;	
				}
	        } 	         				
			row->columns = column_header;
			row->rowid= j;
			row->columncount = m_numbers_of_fields;
			j++;
			mysql_row_result_t* row_header_temp = *result;
			if(row_header_temp == NULL) 
				*result = row;
			else{
				while(row_header_temp->next)
		        {
		            row_header_temp = row_header_temp->next;
		        }
		        row_header_temp->next = row;	
			}
			//	row_header->next = row;
        	printf("\n");  
    	} 		
	}
	//return row_header;		
	//result = row_header;
	mysql_free_result(m_mysql_res);
	return j;
}
long long  insert(int fd,char* sql,...)
{
	mysql_pool_work_t* work = getConn(fd);
	va_list arglist;
	va_start( arglist, sql );
	long long ret = 0;
	pthread_mutex_lock(&work->mysql_opt_lock);
	if( _executeEx(fd,sql,arglist) == 0)
    {
        ret =  mysql_insert_id(work->connect);
    }
   	pthread_mutex_unlock(&work->mysql_opt_lock);
    va_end( arglist );
	return ret;
}


int _executeEx(int fd,char* sqlformat,va_list arglist){
	mysql_pool_work_t* work = getConn(fd);
	char *sql = NULL;
	
	if(arglist != NULL)
	{			
		
		va_list copy;
		va_copy(copy, arglist);
	    size_t length = vsnprintf(NULL, 0, sqlformat, copy);
	    va_end(copy);
		sql = (char *) malloc((length + 1) * sizeof(char));
	    int result = vsnprintf(sql, length + 1, sqlformat, arglist);
	}else{
		
		sql = (char *) malloc((strlen(sqlformat) + 1) * sizeof(char));
		strcpy(sql,sqlformat);
	}
 	
 	int status = mysql_real_query( work->connect, sql, strlen(sql) );
 	printf("%s:%d\n",sql,status);
 	printf("\n");
	if(status)
	{
		unsigned int code = mysql_errno( work->connect );
		const char * desc = mysql_error( work->connect );

		if (code == 2006 || code == 2013)
		{
			//YLOGW("Warning:[%d,%s]", code, desc);
			//YLOGW("ping...");
			status = mysql_ping(work->connect);
			if(status == 0)
			{
				//YLOGW("requery...");
                //mysql_real_query(&m_sqlCon,"set names utf8",strlen("set names utf8"));
                mysql_set_character_set(work->connect,"utf8");
				status = mysql_real_query( work->connect, sql, strlen(sql) );
			}
		}
	}

    if ( status )
    {
        printf("Error:[%d,%s]", mysql_errno( work->connect), mysql_error( work->connect ));
        //printf("previous: %s", prevSQL.c_str());
        //printf("current : %s",  curSQL.c_str());
    }
    free(sql);
	return status;
}
int executeEx(int fd,char* sqlformat,...){
	mysql_pool_work_t* work = getConn(fd);
	va_list arglist;
	va_start( arglist, sqlformat );
	pthread_mutex_lock(&work->mysql_opt_lock);
	int ret = _executeEx(fd,sqlformat,arglist);
	pthread_mutex_unlock(&work->mysql_opt_lock);
	va_end( arglist );
	return ret;
}
int execute(int fd,char* sql){
	mysql_pool_work_t* work = getConn(fd);
	//va_list arglist;
	//va_start( arglist, sql );
	pthread_mutex_lock(&work->mysql_opt_lock);
	int ret = _executeEx(fd,sql,NULL);
	pthread_mutex_unlock(&work->mysql_opt_lock);	
	//va_end( arglist );
	return ret;
}

int getConnect(){
	return getConnection(g_config_default->serverip,
					g_config_default->dbuser,
					g_config_default->pass,
					g_config_default->dbname,
					g_config_default->port,
					g_config_default->attri,
					g_config_default->masterOrSalve);
}
int getConnectByDBType(int masterOrSalve){
	
	return getConnection(g_config_default->serverip,
						g_config_default->dbuser,
						g_config_default->pass,
						g_config_default->dbname,
						g_config_default->port,
						g_config_default->attri,
						masterOrSalve);
}

int getConnection(char* serverip,
					char* dbuser,
					char* pass,
					char* dbname,
				    int port,
					int attri,
					int masterOrSalve){
	mysql_pool_work_t *work, *member;
  	struct timespec outtime;
  	clock_gettime(CLOCK_MONOTONIC, &outtime);
	
	pthread_mutex_lock(&mysql_pook_lock);
	int isNew = 1;
	mysql_pool_work_t* tempWork = queue_head;
	
	while(tempWork){
		//pthread_cond_wait(&rpool->queue_ready, &rpool->queue_lock);
		int ret = pthread_mutex_trylock(&tempWork->mysql_get_lock);
		if(ret == EBUSY){
			tempWork  = tempWork->next;
			continue;
		}				
		else if(ret == 0){					
			isNew = 0;
			pthread_mutex_unlock(&mysql_pook_lock);	
			
			return tempWork->connectid;
		}
		else{ //if(ret == EINVAL){
			printf("the lock is EINVAL\n");
			continue;	
		}
	}	
	
	if(isNew == 1 && mysql_pool_size < max_mysql_pool_size){	
		work = malloc(sizeof(mysql_pool_work_t));
		if (!work) {
		  printf("%smalloc failed\n", __FUNCTION__);
		  pthread_mutex_unlock(&mysql_pook_lock);
		  return -1; 
		}
		//TODO			
//		pthread_mutex_init(&work->mysql_get_lock,NULL);
		pthread_mutex_init(&work->mysql_opt_lock,NULL);
		MYSQL* connect = mysql_create(serverip,
										 dbuser,
										 pass,
										 dbname,
										 port,
										 attri,
										 masterOrSalve);
		if ( NULL == connect)
		{
			//redis为NULL与redis->err是两种不同的错误，若redis->err为true，可使用redis->errstr查看错误信息
			//redisFree(redis);
			free(work);
			//Log("Connect to Mysql Server faile\n");
			pthread_mutex_unlock(&mysql_pook_lock);		
				
			return -1;
		}
		connectid++;
		work->connectid = connectid;
		work->connect = connect;
		strcpy(work->serverip,serverip);
		strcpy(work->dbuser,dbuser);
		strcpy(work->pass,pass);
		strcpy(work->dbname,dbname);
		work->port=port;
		work->masterOrSalve = masterOrSalve;
		work->attri = attri;
		work->next = NULL;
			
		if (pthread_mutex_init(&work->mysql_get_lock, NULL) !=0) {
	   		printf("%s pthread_mutex_init failed, errno%d, error%s\n",
		   __FUNCTION__, errno, strerror(errno));
   			mysql_close(connect);
			free(work);
			pthread_mutex_unlock(&mysql_pook_lock);
			printf("---------------return mysql is NULL\n");
	   		return -1;
   		}
   			
   		/*
	    if (pthread_cond_init(&work->queue_ready, NULL) !=0 ) {
		   printf("%s pthread_cond_init failed, errno%d, error%s\n", 
			   __FUNCTION__, errno, strerror(errno));
		   	redisFree(redis);
			free(work);
			pthread_mutex_unlock(&g_redis_lock);
			printf("---------------return redis is NULL\n");
			*redisId = -1;
		   return NULL;
	    }	  
		*/  
		//pthread_mutex_lock(&rpool->queue_lock);    
			
		member = queue_head;
		if (!member) {
		  queue_head = work;
		} else {
		  while(member->next) {
			  member = member->next;
		  }
		  member->next = work;
		}		
		pthread_mutex_lock(&work->mysql_get_lock);	
		mysql_pool_size++;
		pthread_mutex_unlock(&mysql_pook_lock);	
				
		return work->connectid;		
		/* 通知工作者线程，有新任务添加 */
		//pthread_cond_signal(&rpool->queue_ready);
		//pthread_mutex_unlock(&rpool->queue_lock);
	}
	else if(isNew == 1 ){
			
		int i = 0;
		int max_time_out = 1000;
		while(i<max_time_out){
			tempWork = queue_head;						
			while(tempWork){
				/*
				if(tempWork){
					//outtime.tv_sec = now.tv_sec + 5;
    				outtime.tv_nsec += 100;
					printf("%d to wait ...\n",tempWork->workid);
					pthread_cond_timedwait(&tempWork->queue_ready, &tempWork->queue_lock,outtime);
					printf("%d to wait OK\n",tempWork->workid);				
				}
				else{
					pthread_mutex_unlock(&g_redis_lock);
					*redisId = -1;
					return NULL;				
				}
				*/
				//pthread_cond_wait(&rpool->queue_ready, &rpool->queue_lock);
				int ret = pthread_mutex_trylock(&tempWork->mysql_get_lock);
				if(ret == EBUSY){					
					tempWork   = tempWork->next;
					usleep(10);
					i += 10;
					continue;
				}
				else{
					//Log("get redis id:%d\n",tempWork->connectid);
					pthread_mutex_unlock(&mysql_pook_lock);
					//*redisId = work->workid;
					return tempWork->connectid;
				}
			}	
		}
	//	Log("you need set mysql pool big and big\n");
	//	Log("---------------return mysql is NULL\n");
	
	//	return -1;
	}
	return -1;
	
}
MYSQL* mysql_create(char* serverip,
					char* user,
					char* pass,
					char* dbname, 
					int port,
					int attri,
					int masterOrSalve){
						
/*						
	if(connect != NULL){
		return connect;
	}
	if(serverip== NULL  || strlen(serverip) == 0)
		return NULL;
	if(user == NULL || strlen(user)==0)
		return NULL;
	if(pass == NULL || strlen(pass)==0){
		return NULL;		
	}
	*/
	MYSQL* connect = malloc(sizeof(MYSQL));
	mysql_init(connect);
	
	if (!mysql_real_connect(connect, serverip,user,pass,dbname,port,NULL,attri))
	{
		int error_code = mysql_errno( connect );
		const char *error_desc = mysql_error( connect );		
		printf("Error:[%d,%s]",error_code,error_desc );
		return NULL;
	}
	else
	{
			
		my_bool reconnect = 1;
		mysql_options(connect, MYSQL_OPT_RECONNECT, &reconnect);
        mysql_set_character_set(connect,"utf8");
		return connect;
	}		
		
	return connect;
}