/***************************************************************

 * 模  块：xsamples
 * 文  件：client-select.c
 * 功  能：网络通讯客户端程序，阻塞、有超时。
 * 作  者：阿宝（Po）
 * 日  期：2015-09-27
 * 版  权：Copyright (c) 2012-2015 Dream Company

 ***************************************************************/

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "utils.h"

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

/* member value */
const char *pServerHost = "121.199.44.166";
const unsigned short uServerPort = 32015;
const struct timeval timeo = { 10, 0 };

int main(int argc, char **argv)
{
    int rc;
    int err;
    int sock;
    fd_set rdevent;
    fd_set wrevent;
    fd_set exevent;

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(uServerPort);
#ifdef _WIN32
    svrAddr.sin_addr.S_un.S_addr = inet_addr(pServerHost);
#else
    inet_aton(pServerHost, (struct in_addr *)&serverAddr.sin_addr.s_addr);
#endif /*_WIN32*/

    /* Linux (/etc/protocols) */
    /* 0: internet protocol, pseudo protocol number */
    /* Win32 */
    /* 0: service provider will choose the protocol to use */
    int protocol = 0;
    sock = socket(AF_INET, SOCK_STREAM, protocol);

    /* Linux is -1 */
    /* Win32 is INVALID_SOCKET */
    if (sock == SOCKET_ERROR) {
        err = get_socket_error();
        printf("%s create socket error (%d), %s\n", get_time_string(), err, strerror(err));
        closesocket(sock);
        /* Linux (/usr/include/sysexits.h) */
        return 0;
    }
    printf("%s create socket success\n", get_time_string());

    /* set socket non-block */
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) >= 0) {
            printf("%s set socket non-block success\n", get_time_string());
        }
    }

    /* connect */
    rc = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    err = get_socket_error();
    if (rc == -1 && err != EINPROGRESS) {
        printf("%s connect error (%d), %s\n", get_time_string(), err, strerror(err));
        closesocket(sock);
        return 0;
    }

    /* selcet socket */
    FD_ZERO(&rdevent);
    FD_SET(sock, &rdevent);
    wrevent = rdevent;
    exevent = rdevent;
    rc = wait_for_ready(sock, &rdevent, &wrevent, &exevent, (struct timeval *)&timeo);
    err = get_socket_error();
    if (rc < 0) {
        printf("%s connect failed (%d), %s\n", get_time_string(), err, strerror(err));
        closesocket(sock);
        return 0;
    }

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
            FD_ZERO(&wrevent);
            FD_ZERO(&exevent);
            FD_SET(sock, &wrevent);
            FD_SET(sock, &exevent);
            rc = wait_for_ready(sock, NULL, &wrevent, &exevent, (struct timeval *)&timeo);
            err = get_socket_error();
            if (rc < 0) {
                printf("%s pre-send failed (%d), %s\n", get_time_string(), err, strerror(err));
                closesocket(sock);
                return 0;
            }

            sz = send(sock, buf, left, flags);
            err = get_socket_error();
            if (sz == -1) {
                /* error */
                printf("%s send error (%d), %s\n", get_time_string(), err, strerror(err));
                break;
            }
            else if (sz == 0) {
                /* ??? */
                printf("%s send error (%d), %s\n", get_time_string(), err, strerror(err));
                break;
            }
            left = left - sz;
            printf("%s send success (%d)\n", get_time_string(), sz);
        }

        /* wait for recv able */
        FD_ZERO(&rdevent);
        FD_ZERO(&exevent);
        FD_SET(sock, &rdevent);
        FD_SET(sock, &exevent);
        rc = wait_for_ready(sock, &rdevent, NULL, &exevent, (struct timeval *)&timeo);
        err = get_socket_error();
        if (rc < 0) {
            printf("%s pre-recv failed (%d), %s\n", get_time_string(), err, strerror(err));
            closesocket(sock);
            return 0;
        }

        sz = recv(sock, buf, BUF_LEN, flags);
        err = get_socket_error();
        if (sz == -1) {
            printf("%s recv error (%d), %s\n", get_time_string(), err, strerror(err));
            break;
        }
        else if (sz == 0) {
            printf("%s recv error (%d), %s\n", get_time_string(), err, strerror(err));
            break;
        }
        printf("%s recv success (%d)\n", get_time_string(), sz);

        usleep(1000000);
    }

    if (sock) {
        closesocket(sock);
    }

    return 0;
}
