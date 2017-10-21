#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#define MAX_PTHREADS 64
#define MAX_QUEUE 65536


typedef struct pthreadpool_t pthreadpool_t;

typedef enum {
    // 错误类型
    pthreadpool_invalid_error = 1,
    pthreadpool_lock_error = 2,
    pthreadpool_shutdown_error = 3,
    pthreadpool_queue_full_error = 4,
    threadpool_thread_failure = 5
} pthreadpool_error_t;

typedef enum {
    // 销毁类型
    pthreadpool_imme_destroy = 1,
    pthreadpool_wait_destroy = 2
} pthreadpool_destory_flags_t;



/***
 * 新建线程池
 * @param  pthread_num 需要的线程数目
 * @param  task_queue_num 需要的任务队列数目
 * @return 线程池指针
*/

pthreadpool_t* pthreadpool_create(int pthread_num, int task_queue_num);

/**
 * 增加任务
 * @param pool 线程池指针 
 * @param task 函数指针，表示的是任务的函数
 * @param arg  任务的函数的所需要的参数
 * @return  成功／失败的状态
*/

int pthreadpool_add(pthreadpool_t* pool, void (*function)(void*), void* arg);

/**
 * 销毁线程池
 * @param pool 线程池指针 
 * @param flags 立即销毁还是等所有任务完成之后再销毁
 * @return 成功／失败的状态
*/

int pthreadpool_destory(pthreadpool_t* pool, int flags);


#endif // PTHREAD_POOL_H