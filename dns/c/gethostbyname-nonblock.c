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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#endif /* _WIN32 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>

/* load library */
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif /* _WIN32 */

/* macro define */
#ifdef _WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif /* _MSC_VER || _MSC_EXTENSIONS */
struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};
#else
#endif /* _WIN32 */
#define MAX_IP_LEN 24

/* pre declare */
#ifdef _WIN32
#else
int GetLastError();
#endif /* _WIN32 */
int GetHostByName(char *pszDomain, struct timeval timeo, char **ppIP, int *pLen);
int GetLastDnsError();
char* GetTimeString();

/* member value */
const char dnsHost[] = "114.114.114.114";
const char dnsHostBack[] = "8.8.8.8";
const int dnsPort = 53;
struct timeval sockTimeo = { 30, 0 };
char timeStringBuffer[64] = { 0 };

int main(int argc, char **argv)
{
    int len;
    char host[MAX_IP_LEN] = {0};
    char *pszDomain = "baidu.com";
    len = 0;
    memset(host, 0, MAX_IP_LEN);
    GetHostByName(pszDomain, sockTimeo, (char **)&host, &len);
	return 0;
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
#define MAX_BUF_LEN 512
    char reqBuf[MAX_BUF_LEN] = { 0 };
    char resBuf[MAX_BUF_LEN] = { 0 };
    char *pIP = (char *)ppIP;

    /* request package */
    /* sign flag questions resources authors ext */
    const int headerLen = 12;
    unsigned short int sign = (unsigned short int)rand();

    char *pos = reqBuf;
    pos[0] = (sign & 0xFF00) >> 8;
    pos[1] = sign & 0xFF;
    pos[2] = 1; /* recursion desired */
    pos[5] = 1; /* questions */
    pos += headerLen;

    int domainLen = strlen(pszDomain);
    if (domainLen < 3) {
        printf("%s domain name error\n", GetTimeString());
        return -1;
    }

    char dot = '.';
    char *domainPos = pszDomain;
    unsigned short int sliceLen = 0;
    int i = 0;
    while (i < domainLen) {
        bool isdot = false;
        if (domainPos[0] == dot) {
            isdot = true;
            if (sliceLen == 0) {
                printf("%s domain name error\n", GetTimeString());
                return -1;
            }
        } else {
            isdot = false;
            sliceLen++;
            domainPos++;
        }
        i++;

        if (isdot || i == domainLen)
        {
            /* slice length */
            pos[0] = sliceLen;
            pos += 1;
            /* slice */
            memcpy(pos, domainPos - sliceLen, sliceLen);
            pos += sliceLen;
            domainPos++;
            sliceLen = 0;
        }
    }

    // end of domain
    pos[0] = 0;
    pos += 1;
    // request type
    pos[1] = 1;
    pos += 2;
    // request class
    pos[1] = 1;
    pos += 2;

    // new socket
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

#ifdef _WIN32
#else
#define IPPROTO_UDP 17
#endif

    // socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        err = GetLastError();
        printf("%s create socket error (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return -1;
    }
    printf("%s create socket success\n", GetTimeString());

    /* set socket nonblock */
#ifdef _WIN32
    u_long iMode = 1;
    rc = ioctlsocket(sock, FIONBIO, &iMode);
    if (rc != NO_ERROR)
    {
        err = GetLastError();
        printf("%s ioctl socket error (%d), %s\n", GetTimeString(), err, strerror(err));
    }
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        int err = GetLastError();
        printf("%s get socket flags failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else {
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
            int err = GetLastError();
            printf("%s set socket flags failed (%d), %s\n", GetTimeString(), err, strerror(err));
        }
    }
#endif
    printf("%s set socket nonblock success\n", GetTimeString());

    // select socket
    bool readytosend = false;
    fd_set rdset;
    fd_set wrset;
    fd_set exset;
    FD_ZERO(&wrset);
    FD_ZERO(&exset);
    FD_SET(sock, &wrset);
    FD_SET(sock, &exset);
    struct timeval sockTimeo = {timeo.tv_sec/2, timeo.tv_usec/2};
    rc = select(sock + 1, NULL, &wrset, &exset, &sockTimeo);
    if (rc < 0) {
        err = GetLastError();
        printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else if (rc == 0) {
        err = GetLastError();
        printf("%s select time out (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else if (FD_ISSET(sock, &wrset) && !FD_ISSET(sock, &exset)) {
        printf("%s select success\n", GetTimeString());
        readytosend = true;
    }
    else {
        err = GetLastError();
        printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    if (readytosend) {
        //send to
        struct sockaddr_in svrAddr;
        memset(&svrAddr, 0, sizeof(svrAddr));
        svrAddr.sin_family = AF_INET;
        svrAddr.sin_port = htons(dnsPort);
        unsigned int addrlen = sizeof(svrAddr);
#ifdef _WIN32
        svrAddr.sin_addr.S_un.S_addr = inet_addr(dnsHost);
#else
        inet_aton(dnsHost, (struct in_addr *)&svrAddr.sin_addr.s_addr);
#endif //_WIN32

        int sz = sendto(sock, reqBuf, (pos - reqBuf), 0, (struct sockaddr*)&svrAddr, addrlen);
        err = GetLastError();
        if (sz == -1) {
            printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
        }
        else if (sz == 0) {
            printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
        }
        printf("%s send success (%d)\n", GetTimeString(), sz);

        // wait for recv
        bool readytorecv = false;
        FD_ZERO(&rdset);
        FD_ZERO(&wrset);
        FD_ZERO(&exset);
        FD_SET(sock, &rdset);
        FD_SET(sock, &exset);
        rc = select(sock + 1, &rdset, NULL, &exset, &sockTimeo);
        if (rc < 0) {
            err = GetLastError();
            printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
        }
        else if (rc == 0) {
            err = GetLastError();
            printf("%s select time out (%d), %s\n", GetTimeString(), err, strerror(err));
        }
        else if (FD_ISSET(sock, &rdset) && !FD_ISSET(sock, &exset)) {
            printf("%s select success\n", GetTimeString());
            readytorecv = true;
        }
        else {
            err = GetLastError();
            printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
        }

        if (readytorecv)
        {
            sz = recvfrom(sock, resBuf, MAX_BUF_LEN, 0, (struct sockaddr*)&svrAddr, &addrlen);
            if (sz == -1) {
                err = GetLastError();
                printf("%s recv error (%d), %s\n", GetTimeString(), err, strerror(err));
            }
            else if (sz == 0) {
                int err = GetLastError();
                printf("%s recv error (%d), %s\n", GetTimeString(), err, strerror(err));
            }
            else if (sz <= 12) {
                printf("%s recv data size error (%d)\n", GetTimeString(), sz);
            }
            else {
                printf("%s recv success (%d)\n", GetTimeString(), sz);

                // analysis dns server's response
                pos = resBuf;
                unsigned short resID = ntohs(*(unsigned short*)pos);
                if (resID == sign)
                {
                    printf("%s analysis match my request (%u)\n", GetTimeString(), resID);

                    pos += headerLen;
                    // read domain
                    while (1)
                    {
                        int sliceLen = pos[0];
                        pos++;
                        if (sliceLen > 0)
                        {
                            pos += sliceLen;
                        }
                        else{
                            break;
                        }
                    }
                    // skip type (2B)
                    pos += 2;
                    // skip class (2B)
                    pos += 2;
                    // skip name (2B), what's name ?
                    pos += 2;
                    // type (2B)
                    //unsigned short resType = ntohs(*(unsigned short*)pos);
                    pos += 2;
                    // class (2B)
                    //unsigned short resClass = ntohs(*(unsigned short*)pos);
                    pos += 2;
                    // skip time to live (4B)
                    pos += 4;
                    // data length (2B)
                    //unsigned short resDataLen = ntohs(*(unsigned short*)pos);
                    pos += 2;
                    // address (4B)
                    char addrSliceBuf[4] = { 0 };
                    unsigned int addrSlice = (unsigned char)pos[0];
                    pos++;
                    sprintf(addrSliceBuf, "%u", addrSlice);
                    memcpy(pIP, addrSliceBuf, strlen(addrSliceBuf));
                    pIP += strlen(addrSliceBuf);
                    pIP[0] = dot;
                    pIP++;
                    memset(addrSliceBuf, 0, 4);
                    addrSlice = (unsigned char)pos[0];
                    pos++;
                    sprintf(addrSliceBuf, "%u", addrSlice);
                    memcpy(pIP, addrSliceBuf, strlen(addrSliceBuf));
                    pIP += strlen(addrSliceBuf);
                    pIP[0] = dot;
                    pIP++;
                    memset(addrSliceBuf, 0, 4);
                    addrSlice = (unsigned char)pos[0];
                    pos++;
                    sprintf(addrSliceBuf, "%u", addrSlice);
                    memcpy(pIP, addrSliceBuf, strlen(addrSliceBuf));
                    pIP += strlen(addrSliceBuf);
                    pIP[0] = dot;
                    pIP++;
                    memset(addrSliceBuf, 0, 4);
                    addrSlice = (unsigned char)pos[0];
                    pos++;
                    sprintf(addrSliceBuf, "%u", addrSlice);
                    memcpy(pIP, addrSliceBuf, strlen(addrSliceBuf));
                    pIP += strlen(addrSliceBuf);

                    printf("%s analysis success (%s)\n", GetTimeString(), (char *)ppIP);
                    // need only the first address, skip others
                }
            }

            return 0;
        }
    }

    // server not answer, select again

    return -1;
}

#ifdef _WIN32
#else
int GetLastError()
{
    return errno;
}
#endif //_WIN32

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
