#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

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

/* member value */
const char *pServerHost = "127.0.0.1";
const unsigned short serverPort = 32015;
char timeStringBuffer[64] = { 0 };

int main(int argc, char **argv)
{
    int rc;
    int sock;
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
        exit(0);
    }
    printf("%s create socket success\n", GetTimeString());

    /* set socket nonblock */
    fd_set rdevents;
    fd_set wrevents;
    fd_set exevents;
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

    rc = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc == -1 && GetLastError() != EINPROGRESS) {
        int err = GetLastError();
        printf("%s connect error (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    }

    /* selcet connect */
    FD_ZERO(&rdevents);
    FD_SET(sock, &rdevents);
    wrevents = rdevents;
    exevents = rdevents;
    struct timeval conntv = { 3, 0 };
    rc = select(sock + 1, &rdevents, &wrevents, &exevents, &conntv);
    if (rc < 0) {
        int err = GetLastError();
        printf("%s select failed (%d), %s\n", GetTimeString(), err, strerror(err));
    } else if (rc == 0) {
        int err = GetLastError();
        printf("%s connect time out (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    } else if (IsConnectedNonblock(sock, &rdevents, &wrevents, &exevents)) {
        printf("%s connect success\n", GetTimeString());
    } else {
        int err = GetLastError();
        printf("%s connect failed (%d), %s\n", GetTimeString(), err, strerror(err));
        close(sock);
        return 0;
    }

    if (fcntl(sock, F_SETFL, flags) < 0) {
        int err = GetLastError();
        printf("%s set socket flags failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    /* set timeout */
    struct timeval rdwrtv = { 10, 0 };
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&rdwrtv, sizeof(rdwrtv)) != 0)
    {
        int err = GetLastError();
        printf("%s set socket send timeout failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&rdwrtv, sizeof(rdwrtv)) != 0)
    {
        int err = GetLastError();
        printf("%s set socket receive timeout failed (%d), %s\n", GetTimeString(), err, strerror(err));
    }

    /* disable tcp nagle's algorithm */
    int on = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

#define BUF_LEN 128
    char buf[BUF_LEN] = { 0 };
    /* With a zero flags, send() is equivalent to write() */
    flags = 0;
    while (1) {
        int left = BUF_LEN;
        int sz;

        while (left > 0) {
            sz = send(sock, buf, left, flags);
            if (sz == -1) {
                /* error */
                int err = GetLastError();
                printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
                break;
            } else if (sz == 0) {
                /* ??? */
                int err = GetLastError();
                printf("%s send error (%d), %s\n", GetTimeString(), err, strerror(err));
                break;
            }
            left = left - sz;
            printf("%s send success (%d)\n", GetTimeString(), sz);
        }

        sz = recv(sock, buf, BUF_LEN, flags);
        if (sz == -1) {
            int err = GetLastError();
            printf("%s recv error (%d), %s\n", GetTimeString(), err, strerror(err));
            break;
        } else if (sz == 0) {
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
    int len = sizeof(err);
    errno = 0;
    if (!FD_ISSET(s, rd) && !FD_ISSET(s, wr))
        return 0;
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
        return 0;
    errno = err;
    return (err == 0);
#endif
}
