/***************************************************************

 * 模  块：xsamples
 * 文  件：client.c
 * 功  能：网络通讯客户端程序，阻塞、无超时
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
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

/* member value */
const char *pServerHost = "127.0.0.1";
const unsigned short serverPort = 32015;
char timeStringBuffer[64] = {0};

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
        /* Linux (/usr/include/sysexits.h) */
        exit(0);
    }
    printf("%s create socket success\n", GetTimeString());

    rc = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc == -1) {
        int err = GetLastError();
        printf("%s connect error (%d), %s\n", GetTimeString(), err, strerror(err));
        exit(0);
    }
    printf("%s connect success\n", GetTimeString());

#define BUF_LEN 128
    char buf[BUF_LEN] = {0};
    /* With a zero flags, send() is equivalent to write() */
    int flags = 0;
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

        usleep(1000);
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
	tv_ms = tv.tv_usec/1000;
    
	sprintf(timeStringBuffer, "%d-%d %d:%d:%d.%d", localDate->tm_mon+1, localDate->tm_mday, localDate->tm_hour, localDate->tm_min, localDate->tm_sec, tv_ms);
    return (char*)&timeStringBuffer;
}
