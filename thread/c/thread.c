/***************************************************************

 * 模  块：xsamples
 * 文  件：thread.c
 * 功  能：多线程示例；生产者消费者模型；
 * 作  者：阿宝（Po）
 * 日  期：2015-09-21
 * 版  权：Copyright (c) 2012-2014 Dream Company

***************************************************************/


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>


/* load library */

/* macro define */

/* pre declare */
int GetLastError();
char* GetTimeString();
void *producterThread (void *argv);
void *customerThread(void *argv);

/* member value */
int poolSize = 1024;
int productPool[1024] = {0};
int poolOffset = 0;
char timeStringBuffer[64] = {0};

int main(int argc, char **argv)
{
    int rc;
    int err;
    pthread_t thd;
    pthread_t thd2;
    pthread_attr_t attr;
    /* attr 的用处还没有搞清楚？ */
    rc = pthread_attr_init(&attr);
    if (rc != 0)
    {
        err = GetLastError();
        printf("%s init thread attr failed (%d), %s\n", GetTimeString(), err, strerror(err));
        return 0;
    }

    char *thdname = "producter";
    char *thdname2 = "customer";

    rc = pthread_create(&thd, &attr, &producterThread, (void *)thdname);
    if (rc != 0)
    {
        err = GetLastError();
        printf("%s create producter thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    rc = pthread_create(&thd2, &attr, &customerThread, (void *)thdname2);
    if (rc != 0)
    {
        err = GetLastError();
        printf("%s create customer thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    rc = pthread_join(thd, NULL);
    if (rc != 0) {
        err = GetLastError();
        printf("%s join thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    rc = pthread_join(thd2, NULL);
    if (rc != 0) {
        err = GetLastError();
        printf("%s join thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    pthread_attr_destroy(&attr);

	return 0;
}

/* 生产者线程 */
void *producterThread (void *argv)
{
    //struct thread_info *thdinfo = argv;
    //printf("thread id (%d) name (%s)\n", thdinfo->thread_num, thdinfo->argv_string);

    while (1)
    {
        if (poolOffset < poolSize - 1) {
            int product = rand();
            printf("pool length (%d), push (%d)\n", poolOffset, product);
            productPool[poolOffset] = product;
            poolOffset++;
        } else {
            printf("pool length (%d), full, sleep\n", poolOffset);
            usleep(3000000);
        }
        usleep(900000);
    }
}

/* 消费者线程 */
void *customerThread(void *argv)
{
    //struct thread_info *thdinfo = argv;
    //printf("thread id (%d) name (%s)\n", thdinfo->thread_num, thdinfo->argv_string);

    while (1)
    {
        if (poolOffset > 1) {
            int product = productPool[poolOffset - 1];
            printf("pool length (%d), pull (%d)\n", poolOffset, product);
            productPool[poolOffset] = 0;
            poolOffset--;
        } else {
            printf("pool length (%d), empty, sleep\n", poolOffset);
            usleep(5000000);
        }
        usleep(1200000);
    }
}

int GetLastError()
{
    int err;
#ifdef _WIN32
    err = WSAGetLastError();
#else
    err = errno;
#endif
    return err;
}

char* GetTimeString() {
    /* get local time, without ms */
    time_t utcDate;
    time(&utcDate);
    struct tm *localDate;
    localDate = localtime(&utcDate);
    /* get ms time */
    struct timeval tv;
    int tv_ms;
    gettimeofday(&tv, NULL);
    tv_ms = tv.tv_usec / 1000;

    sprintf(timeStringBuffer, "%d-%d %d:%d:%d.%d", localDate->tm_mon + 1, localDate->tm_mday, localDate->tm_hour, localDate->tm_min, localDate->tm_sec, tv_ms);
    return (char*)&timeStringBuffer;
}
