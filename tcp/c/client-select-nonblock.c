/***************************************************************

* 模  块：xsamples
* 文  件：client.c
* 功  能：网络通讯客户端程序，阻塞、有超时。
* 作  者：阿宝（Po）
* 日  期：2015-09-21
* 版  权：Copyright (c) 2012-2014 Dream Company

***************************************************************/

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

/* load library */
#ifdef _WIN32
#pragma comment（lib，"Ws2_32.lib"）
#endif //_WIN32

/* macro define */
#ifdef _WIN32
#define SOCKET_ERROR INVALID_SOCKET
#else
#define SOCKET_ERROR -1
#endif //_WIN32 && unix

/* pre declare */
int GetLastError();
char* GetTimeString();
int IsConnectedNonblock(int s, fd_set *rd, fd_set *wr, fd_set *ex);
void *socketThread (void *argv);

/* member value */
const char *pServerHost = "121.199.44.166";
const unsigned short serverPort = 32015;
char timeStringBuffer[64] = { 0 };
int  g_uForceCloseFd[2] = {-1, -1};

int main(int argc, char **argv)
{
    int rc;
    int err;

    pthread_t thd;
    pthread_attr_t attr;
    /* attr 的用处还没有搞清楚？ */
    rc = pthread_attr_init(&attr);
    if (rc != 0)
    {
        err = GetLastError();
        printf("%s init thread attr failed (%d), %s\n", GetTimeString(), err, strerror(err));
        return 0;
    }

    char *thdname = "socket";

    rc = pthread_create(&thd, &attr, &socketThread, (void *)thdname);
    if (rc != 0)
    {
        err = GetLastError();
        printf("%s create producter thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    
    sleep(3);
    
    printf("%s force close select\n", GetTimeString());
    
        close(g_uForceCloseFd[0]);
    printf("%s force close select\n", GetTimeString());
    
    rc = pthread_join(thd, NULL);
    if (rc != 0) {
        err = GetLastError();
        printf("%s join thread failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    return 0;
}

void *socketThread (void *argv)
{
    printf("%s start socket thread\n", GetTimeString());
    
    int rc;
    int err;
    int sock;
    close(g_uForceCloseFd[0]);
    close(g_uForceCloseFd[1]);
    if (pipe(g_uForceCloseFd) != 0)
    {
        err = GetLastError();
        printf("%s create pipe failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    printf("%s pipe (%d)\n", GetTimeString(), g_uForceCloseFd[0]);
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_aton(pServerHost, (struct in_addr *)&serverAddr.sin_addr.s_addr);

    /* get a socket descriptor */
    int domain = AF_INET;
    int type = SOCK_STREAM;
    /* Linux (/etc/protocols) */
    /* 0: internet protocol, pseudo protocol number */
    /* Win32 */
    /* 0: service provider will choose the protocol to use */
    int protocol = 0;
    sock = socket(domain, type, protocol);

    /* Linux is -1 */
    /* Win32 is INVALID_SOCKET */
    if (sock == SOCKET_ERROR) {
        int err = GetLastError();
        printf("%s create socket error (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        /* Linux (/usr/include/sysexits.h) */
        return 0;
    }
    printf("%s create socket success\n", GetTimeString());

    /* set socket nonblock */
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

    /* connect */
    rc = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    err = GetLastError();
    if (rc == -1 && err != EINPROGRESS) {
        err = GetLastError();
        printf("%s connect error (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    }

    /* selcet socket */
    fd_set socketRDfd;
    fd_set socketWRfd;
    fd_set socketEXfd;
    FD_ZERO(&socketRDfd);
    FD_ZERO(&socketWRfd);
    FD_ZERO(&socketEXfd);
    FD_SET(sock, &socketRDfd);
    FD_SET(sock, &socketWRfd);
    FD_SET(sock, &socketEXfd);
    struct timeval timeo = { 10, 0 };
    int readFd = g_uForceCloseFd[1];
    FD_SET(readFd, &socketRDfd);
    int fd = (readFd > sock) ? readFd : sock;
    //int fd = readFd;
    printf("%s select\n", GetTimeString());
    rc = select(fd + 1, &socketRDfd, &socketWRfd, &socketEXfd, &timeo);
    if (rc < 0) {
        if (FD_ISSET(readFd, &socketRDfd))
            printf("%s connect wakeup by pipe\n", GetTimeString());
        int err = GetLastError();
        printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    else if (rc == 0) {
        int err = GetLastError();
        printf("%s connect time out (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    }
    else if (IsConnectedNonblock(sock, &socketRDfd, &socketWRfd, &socketEXfd)) {
        printf("%s connect success\n", GetTimeString());
    }
    else {
        if (FD_ISSET(readFd, &socketRDfd))
            printf("%s connect wakeup by pipe\n", GetTimeString());
        int err = GetLastError();
        printf("%s connect failed (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    }
    printf("%s select\n", GetTimeString());

    /* set socket block */
    //if (fcntl(sock, F_SETFL, flags) < 0) {
    //    int err = GetLastError();
    //    printf("%s set socket flags failed (%d), %s\n", GetTimeString(), err, strerror(err));
    //}

    /* set timeout */
    //struct timeval rdwrtv = { 10, 0 };
    //if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&rdwrtv, sizeof(rdwrtv)) != 0)
    //{
    //   int err = GetLastError();
    //    printf("%s set socket send timeout failed (%d), %s\n", GetTimeString(), err, strerror(err));
    //}
    //if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&rdwrtv, sizeof(rdwrtv)) != 0)
    //{
    //    int err = GetLastError();
    //    printf("%s set socket receive timeout failed (%d), %s\n", GetTimeString(), err, strerror(err));
    //}

    /* disable tcp nagle's algorithm */
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

#define BUF_LEN 128
    char buf[BUF_LEN] = { 0 };
    /* With a zero flags, send() is equivalent to write() */
    /* With a zero flags, recv() is equivalent to read() */
    flags = 0;
    while (1) {
        int left = BUF_LEN;
        int sz;

        while (left > 0) {
            /* wait for send able */
            FD_ZERO(&socketRDfd);
            FD_ZERO(&socketWRfd);
            FD_ZERO(&socketEXfd);
            FD_SET(sock, &socketWRfd);
            FD_SET(sock, &socketEXfd);
            FD_SET(g_uForceCloseFd[1], &socketRDfd);
            int fd = (g_uForceCloseFd[1] > sock) ? g_uForceCloseFd[1] : sock;
            rc = select(fd + 1, NULL, &socketWRfd, &socketEXfd, &timeo);
            if (rc < 0) {
                if (FD_ISSET(g_uForceCloseFd[1], &socketRDfd))
                    printf("%s pre-send wakeup by pipe\n", GetTimeString());
                int err = GetLastError();
                printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
                close(sock);
                return 0;
            }
            else if (rc == 0) {
                int err = GetLastError();
                printf("%s pre-send time out (%d), %s\n", GetTimeString(), err, strerror(err));
                close(sock);
                return 0;
            }
            else if (FD_ISSET(sock, &socketWRfd) && !FD_ISSET(sock, &socketEXfd)) {
                printf("%s pre-send success\n", GetTimeString());
            }
            else {
                if (FD_ISSET(g_uForceCloseFd[1], &socketRDfd))
                    printf("%s pre-send wakeup by pipe\n", GetTimeString());
                int err = GetLastError();
                printf("%s pre-send failed (%d), %s\n", GetTimeString(), err, strerror(err));
                close(sock);
                return 0;
            }
            
            sz = send(sock, buf, left, flags);
            err = GetLastError();
            if (sz == -1) {
                /* error */
                printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
                break;
            }
            else if (sz == 0) {
                /* ??? */
                printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
                break;
            }
            left = left - sz;
            printf("%s send success (%d)\n", GetTimeString(), sz);
        }
        
        /* wait for recv able */
        FD_ZERO(&socketRDfd);
        FD_ZERO(&socketWRfd);
        FD_ZERO(&socketEXfd);
        FD_SET(sock, &socketRDfd);
        FD_SET(sock, &socketEXfd);
        FD_SET(g_uForceCloseFd[1], &socketRDfd);
        int fd = (g_uForceCloseFd[1] > sock) ? g_uForceCloseFd[1] : sock;
        rc = select(fd + 1, &socketRDfd, NULL, &socketEXfd, &timeo);
        if (rc < 0) {
            if (FD_ISSET(g_uForceCloseFd[1], &socketRDfd))
                printf("%s pre-recv wakeup by pipe\n", GetTimeString());
            int err = GetLastError();
            printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
            close(sock);
            return 0;
        }
        else if (rc == 0) {
            int err = GetLastError();
            printf("%s pre-recv time out (%d), %s\n", GetTimeString(), err, strerror(err));
            close(sock);
            return 0;
        }
        else if (FD_ISSET(sock, &socketRDfd) && !FD_ISSET(sock, &socketEXfd)) {
            printf("%s pre-recv success\n", GetTimeString());
        }
        else {
            if (FD_ISSET(g_uForceCloseFd[1], &socketRDfd))
                printf("%s pre-recv wakeup by pipe\n", GetTimeString());
            int err = GetLastError();
            printf("%s pre-recv failed (%d), %s\n", GetTimeString(), err, strerror(err));
            close(sock);
            return 0;
        }
        
        sz = recv(sock, buf, BUF_LEN, flags);
        if (sz == -1) {
            int err = GetLastError();
            printf("%s recv error (%d), %s\n", GetTimeString(), err, strerror(err));
            break;
        }
        else if (sz == 0) {
            int err = GetLastError();
            printf("%s recv error (%d), %s\n", GetTimeString(), err, strerror(err));
            break;
        }
        printf("%s recv success (%d)\n", GetTimeString(), sz);

        usleep(1000000);
    }

    if (sock) {
        close(sock);
    }

    return 0;
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

int IsConnectedNonblock(int s, fd_set *rd, fd_set *wr, fd_set *ex) {
#ifdef _WIN32
    WSASetLastError(0);
    if (!FD_ISSET(s, rd) && !FD_ISSET(s, wr))
        return 0;
    if (FD_ISSET(s, ex))
        return 0;
    return 1;
#else
    int err;
    unsigned int len = sizeof(err);
    errno = 0;
    if (!FD_ISSET(s, rd) && !FD_ISSET(s, wr))
        return 0;
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        return 0;
    errno = err;
    return (err == 0);
#endif
}
