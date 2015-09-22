/***************************************************************

 * 模  块：xsamples
 * 文  件：gethostbyname-nonblock.c
 * 功  能：DNS解析，非阻塞，超时
 * 作  者：阿宝（Po）
 * 日  期：2015-09-22
 * 版  权：Copyright (c) 2012-2014 Dream Company

***************************************************************/

#ifdef _WIN32
#include <Winsock2.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif //_WIN32
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "tadns/tadns.h"

/* load library */
#ifdef _WIN32
#pragma comment（lib，"Ws2_32.lib"）
#endif //_WIN32

/* macro define */
#ifdef _WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif //_MSC_VER || _MSC_EXTENSIONS
struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};
#else
#endif //_WIN32
#define MAX_IP_LEN 16

/* pre declare */
#ifdef _WIN32
#else
int GetLastError();
#endif //_WIN32
int GetHostByName(char *pszDomain, struct timeval timeo, char **ppIP, int *pLen);
int GetLastDnsError();
char* GetTimeString();

/* member value */
const char dnsHost[] = "114.114.114.114";
const char dnsHostBack[] = "8.8.8.8";
struct timeval sockTimeo = { 30, 0 };
char timeStringBuffer[64] = { 0 };

int main(int argc, char **argv)
{
    int len;
    char host[MAX_IP_LEN];
    char *pszDomain = "heysound.com";
    len = 0;
    memset(host, 0, MAX_IP_LEN);
    GetHostByName(pszDomain, sockTimeo, &host, &len);
	return 0;
}

static void dns_callback(void *context, enum dns_query_type qtype,
    const char *name, const unsigned char *addr, size_t addrlen)
{
    if (addrlen == 0) {
        (void)fprintf(stderr, "No idea about [%s]\n", name);
    }
    else {
        //char *pIP = (int *)context;
        //int *pLen = (int *)context + 1;
        printf("%u.%u.%u.%u\n", addr[0], addr[1], addr[2], addr[3]);
        //sprintf(pIP, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        //(*pLen) = strlen(pIP);
    }
}

/**
 * 接口功能：域名解析
 * 参    数：
 * 返 回 值：0 成功/ 非0 失败
 **/
int GetHostByName(char *pszDomain, struct timeval timeo, char **ppIP, int *pLen)
{
    int rc;
    int err;
    if (pszDomain == NULL || ppIP == NULL || pLen == NULL)
    {
        printf("%s input arguments error\n", GetTimeString());
        return -1;
    }
    struct dns	*dns;
    fd_set rdset;
    fd_set exset;
    dns = dns_init();
    int sock = dns_get_fd(dns);

    FD_ZERO(&rdset);
    FD_ZERO(&exset);
    FD_SET(sock, &rdset);
    FD_SET(sock, &exset);

    int usrData[2];
    usrData[0] = ppIP;
    usrData[1] = &pLen;

    //dns_queue(dns, (void*)usrData, pszDomain, DNS_A_RECORD, dns_callback);
    dns_queue(dns, NULL, pszDomain, DNS_A_RECORD, dns_callback);
    rc = select(sock + 1, &rdset, NULL, &exset, &sockTimeo);
    if (rc < 0) {
        err = GetLastError();
        printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else if (rc == 0) {
        err = GetLastError();
        printf("%s dns time out (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else if (FD_ISSET(sock, &rdset) && !FD_ISSET(sock, &exset)) {
        dns_poll(dns);
        printf("%s dns success\n", GetTimeString());
        dns_fini(dns);
        return 0;
    }
    else {
        err = GetLastError();
        printf("%s dns failed (%d), %s\n", GetTimeString(), err, strerror(err));
        return -1;
    }

    dns_fini(dns);
    return -1;
}

#ifdef _WIN32
#else
int GetLastError()
{
    return errno;
}
#endif

#ifdef _WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;
    static int tzflag = 0;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        tmpres /= 10;  /*convert into microseconds*/
        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
#else
#endif

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
