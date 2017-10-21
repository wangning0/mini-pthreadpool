#include <stdio.h>
#include <assert.h>
#include "pthreadpool.h"

int main()
{
    pthreadpool_t *pool;
    assert((pool = pthreadpool_create(10, 10)) != NULL);
    // pthreadpool_t *pool = pthreadpool_create(10, 10);
    // printf("pool->pthreads_size: %d\n", pool.pthread_size);
    // printf("pool->queue_size: %d\n", pool->queue_size);

    return 0;
}

