#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "pthreadpool.h"

// 任务结构体
typedef struct 
{
    void(* function)(void *);
    void* arg;
} pthreadpool_task_t;

// 线程池结构体
struct pthreadpool_t
{
    pthread_t *pthreads;  // 线程数组
    pthreadpool_task_t* tasks; // 任务队列
    pthread_cond_t nodify; // 信号量
    pthread_mutex_t mutex; // 互斥锁
    int count; // 任务队列里的任务数，即等待运行的任务数
    int head; // 队列的头指针
    int tail; // 队列的尾指针
    int queue_size; // 队列大小
    int pthread_size; // 线程大小
    int shutdown; // 线程池是否已关闭
    int started_count; // 开始执行的线程数
};

void* pthreadpool_pthread(void *arg);

int pthreadpool_free(pthreadpool_t *pool);

pthreadpool_t* pthreadpool_create(int pthread_num, int task_queue_num)
{ 
    if(pthread_num <= 0 || task_queue_num <= 0 || pthread_num > MAX_PTHREADS || task_queue_num > MAX_QUEUE)
    {
        printf("pthreadpool: pthread_num or task_queue_num illegal\n");
        return NULL;
    }
    pthreadpool_t* pool;
    int i; // 后面的for循环使用的变量
    if((pool = (pthreadpool_t*)malloc(sizeof(pthreadpool_t) * pthread_num)) == NULL)
    {
        printf("pthreadpool: malloc pthreadpool failure;\n");
        return NULL;
    }
    pool->head = pool->tail = pool->count = 0;
    pool->queue_size = task_queue_num;
    pool->pthread_size = 0;
    pool->shutdown = 0;

    if((pool->tasks = (pthreadpool_task_t*)malloc(sizeof(pthreadpool_task_t) * task_queue_num)) == NULL)
    {
        printf("pthreadpool: malloc pthreadpool failure;\n");
        pthreadpool_free(pool);
        return NULL;
    }

    if((pool->pthreads = (pthread_t*)malloc(sizeof(pthread_t) * pthread_num)) == NULL)
    {
        printf("pthreadpool: malloc pthreadpool failure;\n");
        pthreadpool_free(pool);
        return NULL;
    }
    if((pthread_mutex_init(&(pool->mutex), NULL) != 0) || (pthread_cond_init(&(pool->nodify), NULL) != 0))
    {
        printf("pthreadpool: malloc pthreadpool failure;\n");
        pthreadpool_free(pool);
        return NULL;
    }

    for(i = 0; i < pthread_num; i++)
    {
        if(pthread_create(&(pool->pthreads[i]), NULL, pthreadpool_pthread, (void *)pool) != 0)
        {
            printf("pthreadpool: malloc pthreadpool failure;\n");
            pthreadpool_destory(pool, pthreadpool_imme_destroy);
            return NULL;
        }
        pool->started_count++;
    }

    return pool;
}

int pthreadpool_add(pthreadpool_t* pool, void(*function)(void*), void* arg)
{
    if(pool == NULL || function == NULL)
        return pthreadpool_invalid_error;
    int next, err;

    if(pthread_mutex_lock(&(pool->mutex)) != 0)
    {
        return pthreadpool_lock_error;
    }

    next = pool->tail + 1;
    next = (next == pool->queue_size) ? 0 : next;
    // hack
    do{
        if(pool->count == pool->queue_size)
        {
            err = pthreadpool_queue_full_error;
            break;
        }

        if(pool->shutdown)
        {
            err = pthreadpool_shutdown_error;
            break;
        }

        pool->tasks[pool->tail].function = function;
        pool->tasks[pool->tail].arg = arg;

        pool->tail = next;
        pool->count++;

        if(pthread_cond_signal(&(pool->nodify)) != 0)
        {
            err = pthreadpool_lock_error;
            break;
        }
    } while(0);


    if(pthread_mutex_unlock(&(pool->mutex)) != 0)
    {   
        err = pthreadpool_lock_error;
    }

    return err;
}

int pthreadpool_destory(pthreadpool_t* pool, int flags)
{
    int err, i;
    if(pool == NULL)
    {
        return pthreadpool_invalid_error;
    }

    if(pthread_mutex_lock(&(pool->mutex)) != 0)
    {
        return pthreadpool_lock_error;
    }

    do{
        if(pool->shutdown)
        {
            err = pthreadpool_shutdown_error;
            break;
        }

        pool->shutdown = (flags & pthreadpool_imme_destroy) ? pthreadpool_imme_destroy : pthreadpool_wait_destroy;

        if(pthread_cond_broadcast(&(pool->nodify)) != 0 || pthread_mutex_unlock(&(pool->mutex)) != 0)
        {
            err = pthreadpool_lock_error;
            break; 
        }

        for(i = 0; i < pool->pthread_size; i++)
        {
            if(pthread_join(pool->pthreads[i], NULL) != 0)
            {
                err = threadpool_thread_failure;
            }
        }
    }while(0);

    if(!err)
    {
        pthreadpool_free(pool);
    }
    return err;

}
void* pthreadpool_pthread(void *threadpool)
{
    pthreadpool_t* pool = (pthreadpool_t *)threadpool;
    pthreadpool_task_t task;

    for(;;)
    {
        pthread_mutex_lock(&(pool->mutex));

        while((pool->count == 0) && !(pool->shutdown))
        {
            pthread_cond_wait(&(pool->nodify), &(pool->mutex));
        }

        if(pool->shutdown == pthreadpool_imme_destroy || (pool->shutdown == pthreadpool_wait_destroy && pool->count == 0))
        {
            break;
        }

        task.function = pool->tasks[pool->head].function;
        task.arg = pool->tasks[pool->head].arg;

        pool->head += 1;
        pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->mutex));
        (*(task.function))(task.arg);
    }
    pool->pthread_size++;
    pool->started_count--;

    pthread_mutex_unlock(&(pool->mutex));
    pthread_exit(NULL);
    return NULL;
}


int pthreadpool_free(pthreadpool_t *pool)
{
    // 释放线程池资源
    if(pool == NULL || pool->started_count > 0)
    {
        return -1;
    }

    if(pool->pthreads)
    {
        free(pool->pthreads);
        free(pool->tasks);

        pthread_mutex_lock(&(pool->mutex));
        pthread_mutex_destroy(&(pool->mutex));
        pthread_cond_destroy(&(pool->nodify));
    }
    free(pool);

    return 0;
}