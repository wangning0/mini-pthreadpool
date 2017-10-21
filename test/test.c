#include <stdio.h>
#include "../src/pthreadpool.h"

int main()
{
    pthreadpool_t *pool = pthreadpool_create(10, 10);

    return 0;
}