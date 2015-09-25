/***************************************************************

 * 模  块：DNS解析
 * 文  件：gethostbyname.c
 * 功  能：解析DNS A记录
 * 作  者：阿宝（Po）
 * 日  期：2015-09-24
 * 版  权：Copyright (c) 2012-2014 Dream Company

***************************************************************/
#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif /* _WIN32 */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "gethostbyname.h"

/* load library */
#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#endif /* _WIN32 */

/* macro define */
struct DNS_PACKAGE {
    char *data;
    unsigned int datalen;
};

struct DNS_HEADER {
    unsigned short id;
    unsigned short flag;
    unsigned short questions;
    unsigned short resources;
    unsigned short authors;
    unsigned short exts;
};

struct DNS_QUESTION {
    char *name;
    unsigned int namelen;
    unsigned short type;
    unsigned short clas;
};

struct DNS_RESOURCE {
    unsigned short name;
    unsigned short type;
    unsigned short clas;
    unsigned int ttl;
    unsigned short datalen;
    char *data;
};

enum RR_TYPE
{
    RR_TYPE_A = 1,
    RR_TYPE_MX = 15
};

#ifdef _WIN32
struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};
#define SOCK_NONBLOCK 0
#else
#define closesocket(s) close(s)
#endif /*_WIN32*/
#define MAX_UDP_MSG_SIZE 512
#define MAX_MSG_SIZE 512
#define DNS_HEADER_LEN 12
#define DNS_S_OK 0
#define DNS_E_NULL -1
#define DNS_E_TIMEO -2

/* pre declare */
#ifdef _WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz);
#else
int GetLastError();
#endif /* _WIN32 */
void init_dns_package(struct DNS_PACKAGE *dnsPackage);
void release_dns_package(struct DNS_PACKAGE *dnsPackage);
int create_socket_nonblock();
struct DNS_QUESTION pack_query_question(const char *name);
int wait_for_ready(int s, fd_set *rd, fd_set *wr, fd_set *ex);
int unpack_response_resource_record(struct DNS_PACKAGE dnsPackage, struct IP *ip);
char* gettimestring();

/* member value */
const char dnsHost[] = "120.26.109.136";
const char dnsHostBack[] = "120.26.109.136";
const unsigned short dnsPort = 53;
const char defaultAQueryHeader[DNS_HEADER_LEN] = { 0x43, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
char timeStringBuffer[MAX_MSG_SIZE] = { 0 };
const struct timeval defaultTimeout = { 6, 0 };
static int  g_iCancelSign[2] = { -1, -1 };

void ForceCloseGetHostByName()
{
    close(g_iCancelSign[0]);
}

/*  0: success */
/* ~0: error */
int GetHostByName(const char *name, struct IP *ip)
{
    if (name == NULL || ip == NULL || strlen(name) < 3) {
        printf("%s domain name error\n", gettimestring());
        return DNS_E_NULL;
    }

    if (pipe(g_iCancelSign) != 0)
    {
        printf("%s create pipe failed (%d), %s\n", gettimestring(), GetLastError(), strerror(GetLastError()));
    }

    struct IP resIP;
    memset(resIP.ip, 0, MAX_IP_LEN);
    struct DNS_PACKAGE dnsPackage = {NULL, 0};
    init_dns_package(&dnsPackage);
    /* header */
    struct DNS_HEADER dnsHeader;
    memcpy((void*)&dnsHeader, defaultAQueryHeader, DNS_HEADER_LEN);
    /* question */
    struct DNS_QUESTION dnsQuestion = pack_query_question(name);
    /* pack query package */
    memcpy(dnsPackage.data, (void*)&dnsHeader, DNS_HEADER_LEN);
    dnsPackage.datalen += DNS_HEADER_LEN;
    memcpy(dnsPackage.data + dnsPackage.datalen, dnsQuestion.name, dnsQuestion.namelen);
    dnsPackage.datalen += dnsQuestion.namelen;
    unsigned short type = htons(dnsQuestion.type);
    memcpy(dnsPackage.data + dnsPackage.datalen, (void*)&type, sizeof(type));
    dnsPackage.datalen += sizeof(type);
    unsigned short clas = htons(dnsQuestion.clas);
    memcpy(dnsPackage.data + dnsPackage.datalen, (void*)&clas, sizeof(clas));
    dnsPackage.datalen += sizeof(clas);
    free(dnsQuestion.name);
    dnsQuestion.name = NULL;

    /* dns server address */
    struct sockaddr_in svrAddr;
    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_port = htons(dnsPort);
    unsigned int addrlen = sizeof(svrAddr);
#ifdef _WIN32
    svrAddr.sin_addr.S_un.S_addr = inet_addr(dnsHost);
#else
    inet_aton(dnsHost, (struct in_addr *)&svrAddr.sin_addr.s_addr);
#endif /*_WIN32*/

    /* create socket */
    int sz;
    int rc;
    int err;
    int sock = create_socket_nonblock();
    if (sock < 0)
    {
        release_dns_package(&dnsPackage);
        return DNS_E_NULL;
    }

    fd_set rdset;
    fd_set wrset;
    fd_set exset;

    int retry = 0;
    int ready = 0;
    for (; retry < 2; retry++)
    {
        if (retry == 1)
        {
#ifdef _WIN32
            svrAddr.sin_addr.S_un.S_addr = inet_addr(dnsHostBack);
#else
            inet_aton(dnsHostBack, (struct in_addr *)&svrAddr.sin_addr.s_addr);
#endif /*_WIN32*/
        }

        /* wait for write */
        FD_ZERO(&wrset);
        FD_ZERO(&exset);
        FD_SET(sock, &wrset);
        FD_SET(sock, &exset);
        ready = wait_for_ready(sock, NULL, &wrset, &exset);
        if (ready >= 0)
        {
            /* send query request */
            sz = sendto(sock, dnsPackage.data, dnsPackage.datalen, 0, (struct sockaddr*)&svrAddr, addrlen);
            if (sz > 0) 
            {
                printf("%s send success (%d)\n", gettimestring(), sz);
                /* wait for read */
                FD_ZERO(&rdset);
                FD_ZERO(&exset);
                FD_SET(sock, &rdset);
                FD_SET(sock, &exset);
                ready = wait_for_ready(sock, &rdset, NULL, &exset);
                if (ready >= 0)
                {
                    /* recv response */
                    memset(dnsPackage.data, 0, MAX_UDP_MSG_SIZE);
                    sz = recvfrom(sock, dnsPackage.data, MAX_UDP_MSG_SIZE, 0, (struct sockaddr*)&svrAddr, &addrlen);
                    if (sz > 0)
                    {
                        printf("%s recv success (%d)\n", gettimestring(), sz);
                        dnsPackage.datalen = sz;
                        /* unpack response */
                        rc = unpack_response_resource_record(dnsPackage, &resIP);
                        if (rc == 0)
                        {
                            memcpy(ip->ip, resIP.ip, MAX_IP_LEN);
                            ip->len = resIP.len;
                            release_dns_package(&dnsPackage);
                            return DNS_S_OK;
                        }
                    }
                    else
                    {
                        err = GetLastError();
                        printf("%s recv response error (%d), %s\n", gettimestring(), err, strerror(err));
                    }
                }
            }
            else
            {
                err = GetLastError();
                printf("%s send query error (%d), %s\n", gettimestring(), err, strerror(err));
            }
        }
    }
    release_dns_package(&dnsPackage);
    return DNS_E_NULL;
}

void init_dns_package(struct DNS_PACKAGE *dnsPackage)
{
    if (dnsPackage)
    {
        dnsPackage->data = malloc(MAX_UDP_MSG_SIZE);
        memset(dnsPackage->data, 0, MAX_UDP_MSG_SIZE);
        dnsPackage->datalen = 0;
    }
}
void release_dns_package(struct DNS_PACKAGE *dnsPackage)
{
    if (dnsPackage)
    {
        if (dnsPackage->data)
            free(dnsPackage->data);
        dnsPackage->data = NULL;
        dnsPackage->datalen = 0;
    }
}

int create_socket_nonblock()
{
    int sock;
    /* socket, non-block */
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    int err;
    /* SOCK_NONBLOCK: linux kernel >= 2.6.27 */
    sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP);
    if (sock == -1) {
        err = GetLastError();
        printf("%s create socket error (%d), %s\n", gettimestring(), err, strerror(err));
        closesocket(sock);
        return -1;
    }
    printf("%s create socket success\n", gettimestring());
    /* win32: set socket non-block */
#ifdef _WIN32
    u_long iMode = 1;
    if (ioctlsocket(sock, FIONBIO, &iMode) != NO_ERROR)
    {
        err = GetLastError();
        printf("%s ioctl socket error (%d), %s\n", gettimestring(), err, strerror(err));
        closesocket(sock);
        return -1;
    }
    else
        printf("%s set socket non-block success\n", gettimestring());
#else
    printf("%s set socket non-block success\n", gettimestring());
#endif
    return sock;
}

struct DNS_QUESTION pack_query_question(const char *name)
{
    struct DNS_QUESTION dnsQuestion = { NULL, 0, 0, 0 };
    char dot = '.';
    int nameLen = strlen(name);
    if (name == NULL || nameLen < 3 || name[0] == dot) {
        printf("%s domain name error\n", gettimestring());
        return dnsQuestion;
    }
    dnsQuestion.name = malloc(MAX_UDP_MSG_SIZE);
    char *pos = dnsQuestion.name;
    char *namePos = (char *)name;
    int sliceLen = 0;
    int i = 0;
    while (i < nameLen) {
        bool isdot = false;
        if (namePos[0] == dot) {
            isdot = true;
        } else {
            isdot = false;
            sliceLen++;
            namePos++;
        }
        i++;

        if (isdot || i == nameLen)
        {
            /* slice length */
            pos[0] = sliceLen;
            pos += 1;
            /* slice */
            memcpy(pos, namePos - sliceLen, sliceLen);
            pos += sliceLen;
            namePos++;
            sliceLen = 0;
        }
    }
    /* end of name */
    pos[0] = 0;
    pos += 1;

    dnsQuestion.namelen = (unsigned int)(pos - dnsQuestion.name);
    dnsQuestion.type = RR_TYPE_A;
    dnsQuestion.clas = 1;
    return dnsQuestion;
}

/*  0: ready */
/* -1: error */
/* -2: time out */
int wait_for_ready(int s, fd_set *rd, fd_set *wr, fd_set *ex)
{
    /* read and write, use half of timeo */
    struct timeval timeo = { defaultTimeout.tv_sec / 2, defaultTimeout.tv_usec / 2 };
    int err;
    bool rdisnull = false;
    if (rd == NULL)
    {
        rdisnull = true;
        fd_set rdset;
        FD_ZERO(&rdset);
        rd = &rdset;
    }
    FD_SET(g_iCancelSign[1], rd);
    int fd = (g_iCancelSign[1] > s) ? g_iCancelSign[1] : s;
    int rc = select(fd + 1, rd, wr, ex, &timeo);
    if (rc < 0) {
        if (FD_ISSET(g_iCancelSign[1], rd))
            printf("%s force wakeup select\n", gettimestring());
        err = GetLastError();
        printf("%s select failed (%d), %s\n", gettimestring(), err, strerror(err));
        return DNS_E_NULL;
    }
    else if (rc == 0) {
        err = GetLastError();
        printf("%s select time out (%d), %s\n", gettimestring(), err, strerror(err));
        return DNS_E_TIMEO;
    }
    else {
        bool ready = true;
        if (rd && !rdisnull)
            ready = ready && FD_ISSET(s, rd);
        if (wr)
            ready = ready && FD_ISSET(s, wr);
        if (ex)
            ready = ready && !FD_ISSET(s, ex);

        if (ready)
        {
            printf("%s select success\n", gettimestring());
            return DNS_S_OK;
        }
        else
        {
            err = GetLastError();
            printf("%s select failed (%d), %s\n", gettimestring(), err, strerror(err));
            return DNS_E_NULL;
        }
    }
    return DNS_E_NULL;
}

/*  0: success */
/* ~0: error */
int unpack_response_resource_record(struct DNS_PACKAGE dnsPackage, struct IP *ip)
{
    if (dnsPackage.data == NULL || dnsPackage.datalen < DNS_HEADER_LEN || ip == NULL) {
        printf("%s unpack argument error\n", gettimestring());
        return -1;
    }
    char *pos = dnsPackage.data;
    char *ipPos = ip->ip;
    /* response id match query id */
    if (pos[0] == defaultAQueryHeader[0] && pos[1] == defaultAQueryHeader[1])
    {
        pos += DNS_HEADER_LEN;
        /* read domain name */
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
        /* skip type (2B) + class (2B) + name (2B) + type (2B) + class (2B) + ttl (4B) + length (2B) */
        pos += 16;
        char addrSliceBuf[4] = { 0 };
        unsigned int addrSlice;
        char dot = '.';
        int i = 0;
        for (; i < 4; i++)
        {
            memset(addrSliceBuf, 0, 4);
            addrSlice = (unsigned char)pos[0];
            pos++;
            sprintf(addrSliceBuf, "%u", addrSlice);
            memcpy(ipPos, addrSliceBuf, strlen(addrSliceBuf));
            ipPos += strlen(addrSliceBuf);
            if (i < 3)
            {
                ipPos[0] = dot;
                ipPos++;
            }
        }
        /* need only the first address, skip others */
        return 0;
    }
    return -1;
}

#ifdef _WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (tv)
    {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
        return -1;
    }
    DWORD tick = GetTickCount();
    long ms = (long)(tick % 1000);
    tv->tv_usec = ms * 1000;
    return 0;
}
#else
int GetLastError()
{
    return errno;
}
#endif

char* gettimestring() 
{
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
    memset(timeStringBuffer, 0, MAX_MSG_SIZE);
    sprintf(timeStringBuffer, "%d-%d %d:%d:%d.%d", localDate->tm_mon + 1, localDate->tm_mday, localDate->tm_hour, localDate->tm_min, localDate->tm_sec, tv_ms);
    return (char*)&timeStringBuffer;
}
