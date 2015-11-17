#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "thread_pool.h"
//http://www.cnblogs.com/newth/archive/2012/05/09/2492459.html
static tpool_t *tpool = NULL;
static int max_active_size = 32;
static int max_queue = 0;
static int queue_size = 0;
static int now_thread_count=10;
//static int now_queue = 0;
/* 工作者线程函数, 从任务链表中取出任务并执行 */
static void*
thread_routine(void *arg)
{
    pthread_detach(pthread_self());
    tpool_work_t *work;
    while(1)
    {
        /* 如果线程池没有被销毁且没有任务要执行，则等待 */
        pthread_mutex_lock(&tpool->queue_lock);
        while(!tpool->queue_head && !tpool->shutdown)
        {
            pthread_cond_wait(&tpool->queue_ready, &tpool->queue_lock);
        }
        if (tpool->shutdown)
        {
            pthread_mutex_unlock(&tpool->queue_lock);
            pthread_exit(NULL);
        }
        work = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;
        pthread_mutex_unlock(&tpool->queue_lock);
        work->routine(work->arg);
        free(work);
        queue_size--;
        if(queue_size < max_active_size && now_thread_count > max_active_size )
        {
            //active_size--;
             now_thread_count--;
			 pthread_exit(NULL);
        }
    }

    return NULL;
}

/*
* 创建线程池
*/
int
tpool_create(int max_thr_num)
{
    int i;

    tpool = calloc(1, sizeof(tpool_t));
    if (!tpool)
    {
        printf("%s calloc failed\n", __FUNCTION__);
        exit(1);
    }

    /* 初始化 */
    tpool->max_thr_num = max_thr_num;
    tpool->shutdown = 0;
    tpool->queue_head = NULL;
    if (pthread_mutex_init(&tpool->queue_lock, NULL) !=0)
    {
        printf("%s pthread_mutex_init failed, errno%d, error%s\n",
               __FUNCTION__, errno, strerror(errno));
        exit(1);
    }
    if (pthread_cond_init(&tpool->queue_ready, NULL) !=0 )
    {
        printf("%s pthread_cond_init failed, errno%d, error%s\n",
               __FUNCTION__, errno, strerror(errno));
        exit(1);
    }

    /* 创建工作者线程 */
    tpool->thr_id = calloc(max_thr_num, sizeof(pthread_t));
    if (!tpool->thr_id)
    {
        printf("%s calloc failed\n", __FUNCTION__);
        exit(1);
    }
    for (i = 0; i < max_thr_num; ++i) {
   		tpool->thr_id[i] = 0;
    }
    for (i = 0; i < 10; ++i)
    {
        if (pthread_create(&tpool->thr_id[i], NULL, thread_routine, NULL) != 0)
        {
            printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
                   errno, strerror(errno));
            exit(1);
        }

    }

    return 0;
}

/* 销毁线程池 */
void
tpool_destroy()
{
    int i;
    tpool_work_t *member;

    if (tpool->shutdown)
    {
        return;
    }
    tpool->shutdown = 1;

    /* 通知所有正在等待的线程 */
    pthread_mutex_lock(&tpool->queue_lock);
    pthread_cond_broadcast(&tpool->queue_ready);
    pthread_mutex_unlock(&tpool->queue_lock);
    for (i = 0; i < tpool->max_thr_num; ++i)
    {
        if(tpool->thr_id[i] > 0)
            pthread_join(tpool->thr_id[i], NULL);
    }
    free(tpool->thr_id);

    while(tpool->queue_head)
    {
        member = tpool->queue_head;
        tpool->queue_head = tpool->queue_head->next;
        free(member);
    }

    pthread_mutex_destroy(&tpool->queue_lock);
    pthread_cond_destroy(&tpool->queue_ready);

    free(tpool);
}

/* 向线程池添加任务 */
int
tpool_add_work(void*(*routine)(void*), void *arg)
{
    tpool_work_t *work, *member;
    queue_size++;
    if (!routine)
    {
        printf("%sInvalid argument\n", __FUNCTION__);
        return -1;
    }

    work = malloc(sizeof(tpool_work_t));
    if (!work)
    {
        printf("%smalloc failed\n", __FUNCTION__);
        return -1;
    }
    work->routine = routine;
    work->arg = arg;
    work->next = NULL;

    pthread_mutex_lock(&tpool->queue_lock);
    member = tpool->queue_head;
    if (!member)
    {
        tpool->queue_head = work;
    }
    else
    {
        while(member->next)
        {
            member = member->next;
        }
        member->next = work;
    }
    int i = 0;
    if(queue_size>=max_active_size)
    {
        for (i = 0; i < tpool->max_thr_num; ++i)
        {
        	//printf("tpool->thr_id[i]:%d,%d\n",i,tpool->thr_id[i]);
        	int neednew = 0;
        	if(tpool->thr_id[i] == 0)
				neednew = 1;
			else{
				int kill_rc =pthread_kill(tpool->thr_id[i],0);
				if(kill_rc == ESRCH)
					neednew = 1;	
			}
            
            if(neednew == 1)// == ESRCH)
            {
                if (pthread_create(&tpool->thr_id[i], NULL, thread_routine, NULL) != 0)
                {
                    printf("%spthread_create failed, errno%d, error%s\n", __FUNCTION__,
                           errno, strerror(errno));
                    exit(1);
                }
                now_thread_count++;
                break;
            }
        }
    }
    //int pthread_kill(pthread_t thread, int sig);
    /* 通知工作者线程，有新任务添加 */
    pthread_cond_signal(&tpool->queue_ready);
    pthread_mutex_unlock(&tpool->queue_lock);

    return 0;
}