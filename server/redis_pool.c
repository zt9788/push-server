#include <hiredis/hiredis.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#ifdef CHECKMEM
#include "leak_detector_c.h"
#endif 
#include "log.h"

typedef struct redis_pool_work {
   redisContext         *redis;                    /* 传入任务函数的参数 */
   pthread_mutex_t queue_lock;                    
   pthread_cond_t  queue_ready;    
   int workid;
   struct redis_pool_work   *next;                    
}redis_pool_work_t;

//static redis_pool_work_t* rpool = NULL;
static redis_pool_work_t* queue_head = NULL;
static int pool_size = 0;
static int max_pool_size = 0;
static char g_redis_server[20];
static int g_redis_port = -1;
static pthread_mutex_t g_redis_lock;
int createRedisPool(char* redis_server,int redis_port,int max_thr_num){
   //rpool = calloc(1, sizeof(redis_pool_work_t));
   //if (!rpool) {
//	   printf("%s calloc failed\n", __FUNCTION__);
//	   return 1;
 //  }
   max_pool_size = max_thr_num;
   strcpy(g_redis_server,redis_server);
   g_redis_port = redis_port;
   
   queue_head = NULL;
   pthread_mutex_init(&g_redis_lock, NULL);
   return 0;
}

redisContext* getRedis(int* redisId){
		
	redis_pool_work_t *work, *member;
  	struct timespec outtime;
  	clock_gettime(CLOCK_MONOTONIC, &outtime);
	
	pthread_mutex_lock(&g_redis_lock);
	int isNew = 1;
	redis_pool_work_t* tempWork = queue_head;
	while(tempWork){
		//pthread_cond_wait(&rpool->queue_ready, &rpool->queue_lock);
		int ret = pthread_mutex_trylock(&tempWork->queue_lock);
		if(ret == EBUSY){
			tempWork  = tempWork->next;
			continue;
		}				
		else if(ret == 0){					
			isNew = 0;
			pthread_mutex_unlock(&g_redis_lock);
			//Log("get redis id:%d\n",tempWork->workid);
			*redisId = tempWork->workid;			
			return tempWork->redis;
		}
		else{ //if(ret == EINVAL){
			printf("the lock is EINVAL\n");
			continue;	
		}
	}	
	if(isNew == 1 && pool_size < max_pool_size){	
		work = malloc(sizeof(redis_pool_work_t));
		if (!work) {
		  printf("%smalloc failed\n", __FUNCTION__);
		  pthread_mutex_unlock(&g_redis_lock);
		  *redisId = -1;
		  return NULL; 
		}			
		redisContext* redis = redisConnect(g_redis_server, g_redis_port);
		if ( NULL == redis || redis->err)
		{
			//redis为NULL与redis->err是两种不同的错误，若redis->err为true，可使用redis->errstr查看错误信息
			//redisFree(redis);
			free(work);
			Log("Connect to redisServer faile\n");
			pthread_mutex_unlock(&g_redis_lock);
			*redisId = -1;
			return NULL;
		}
		redisEnableKeepAlive(redis);
		
		work->workid = pool_size;
		work->redis = redis;
		work->next = NULL;
		if (pthread_mutex_init(&work->queue_lock, NULL) !=0) {
	   		printf("%s pthread_mutex_init failed, errno%d, error%s\n",
		   __FUNCTION__, errno, strerror(errno));
   			redisFree(redis);
			free(work);
			pthread_mutex_unlock(&g_redis_lock);
			printf("---------------return redis is NULL\n");
			*redisId = -1;
	   		return NULL;
   		}
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
		pthread_mutex_lock(&work->queue_lock);	
		pool_size++;		
		//Log("get redis id:%d\n",work->workid);
		pthread_mutex_unlock(&g_redis_lock);
		*redisId = work->workid;
		return work->redis;		
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
				int ret = pthread_mutex_trylock(&tempWork->queue_lock);
				if(ret == EBUSY){
					
					tempWork   = tempWork->next;
					usleep(10);
					i += 10;
					continue;
				}
				else{
					//Log("get redis id:%d\n",tempWork->workid);
					pthread_mutex_unlock(&g_redis_lock);
					*redisId = work->workid;
					return tempWork->redis;
				}
			}	
		}
		Log("you need set redis pool big and big\n");
		Log("---------------return redis is NULL\n");
		*redisId = -1;
		return NULL;
	}
	return NULL;
}

void returnRedis (int redisID){
	redis_pool_work_t* tempWork = queue_head;
	//printf("this want to return redis %d\n",redisID);
	while(tempWork){		
		//printf("work id:%d,return id:%d\n",tempWork->workid,redisID);
		if(tempWork->workid == redisID){						
			//pthread_cond_signal(&tempWork->queue_ready);	
			pthread_mutex_unlock(&tempWork->queue_lock);	   	
		   	//
		   	//pool_size--;
		   	//printf("retrun redis workid:%d\n",tempWork->workid);
			break;
		}	
		tempWork  = tempWork->next;
	}
}
void destroy_redis_pool(){
	
	redis_pool_work_t* work = queue_head;
	while(work){	
		if(work->redis){
			redisFree(work->redis);	
			work->redis = NULL;		
		}
		
		pthread_cond_signal(&work->queue_ready);
	   	pthread_mutex_unlock(&work->queue_lock);
	   	pthread_mutex_destroy(&work->queue_lock);    
		pthread_cond_destroy(&work->queue_ready);
		redis_pool_work_t* temp = work;
		work   = work->next;
		free(temp);				
		continue;		
	}	   
	max_pool_size = 0;
	pool_size = 0;
	
}