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
#define MAX_IP_LEN 24

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
const int dnsPort = 53;
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
    char reqBuf[512] = {0};
    int reqLen = 0;
    char resBuf[512] = {0};
    int resLen = 0;
    
    // request package
    // sign flag questions resources authors ext
    unsigned short int sign = (unsigned short int)rand();
    unsigned short int flag = 0x10;
    unsigned short int questions = 1;
    unsigned short int resources = 0;
    unsigned short int authors = 0;
    unsigned short int exts = 0;
    
    char *pos = reqBuf;
    memcpy(pos, &sign, sizeof(sign));
    memcpy(pos, &flag, sizeof(flag));
    memcpy(pos, &questions, sizeof(questions));
    memcpy(pos, &resources, sizeof(resources));
    memcpy(pos, &authors, sizeof(authors));
    memcpy(pos, &exts, sizeof(exts));
    pos += 12;
    
    int domainLen = strlen(pszDomain);
    if (domainLen <= 0) {
        printf("%s domain name error\n", GetTimeString());
        return -1;
    }
    
    char dot = '.';
    char *domainPos = pszDomain;
    unsigned short int sliceLen = 0;
    while (1) {
        if (domainLen <= 0) {
            break;
        }
        sliceLen = 0;
        for (int i = 0; i < domainLen; i++) {
            if (domainPos[i] == dot) {
                if (sliceLen == 0) {
                    break;
                } else {
                }
            } else {
                sliceLen++;
            }
        }
        
        if (sliceLen > 0) {
            // add to request buffer
            pos[0] = sliceLen;
            pos += 1;
            memcpy(pos, domainPos, sliceLen);
            pos += sliceLen;
            // move pos, change length
            domainPos += sliceLen;
            domainLen -= sliceLen;
            
        }
        if (domainPos[0] == dot) {
            domainPos += 1;
            domainLen -= 1;
        }
    }
    
    if (pos - reqBuf <= 12) {
        printf("%s domain name error\n", GetTimeString());
        return -1;
    }
    
    // end of domain
    pos[0] = 0;
    pos += 1;
    
    unsigned short int requesttype = 1;
    unsigned short int requestclass = 1;
    memcpy(pos, &requesttype, sizeof(requesttype));
    pos += 2;
    memcpy(pos, &requestclass, sizeof(requestclass));
    pos += 2;
    
    // new socket
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    
    // nonblock socket
    int sock = socket(AF_INET, SOCK_DGRAM, 17);
    if (sock == SOCKET_ERROR) {
        err = GetLastError();
        printf("%s create socket error (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return -1;
    }
    printf("%s create socket success\n", GetTimeString());
    
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
        memset(&svrAddr, 0, sizeof(sockaddr_in));
        int addr_len = sizeof(sockaddr);
        svrAddr.sin_family = AF_INET;
        svrAddr.sin_port = htons(m_uDNSPort);
#ifdef _WIN32
        svrAddr.sin_addr.S_un.S_addr = inet_addr(dnsHost);
#else
        inet_aton(dnsHost, (struct in_addr *)&vrAddr.sin_addr.S_un.S_addr);
#endif //_WIN32
        
        //sendto(m_sock, m_pData, m_uDataLen, 0, (sockaddr*)&svrAddr, addr_len)
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
