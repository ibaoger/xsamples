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


/* load library */

/* macro define */

/* pre declare */
char* GetTimeString();

/* member value */

int main(int argc, char **argv)
{
    int rc;
    int err;
    int thd;
    int thd2;
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

    pthread_attr_destroy(&attr);

	return 0;
}

/* 生产者线程 */
void *producterThread (void *argv)
{
    while (true)
    {
        usleep(1000);
    }
}

/* 消费者线程 */
void *customerThread(void *argv)
{
    while (true)
    {
        usleep(1000);
    }
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
