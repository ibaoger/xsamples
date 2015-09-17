#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

/* member value */
const char *pServerHost = "127.0.0.1";
const unsigned short serverPort = 32015;

int main(int argc, char **argv)
{
    int rc;
    int sock;
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_aton(pServerHost, &serverAddr.sin_addr.s_addr);

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
        printf("create socket error (%d), %s\n", err, strerror(err));
        /* Linux (/usr/include/sysexits.h) */
        exit(0);
    }
    printf("create socket success\n");

    rc = connect(sock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (rc == -1) {
        int err = GetLastError();
        printf("connect error (%d), %s\n", err, strerror(err));
        exit(0);
    }
    printf("connect success\n");

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
                int err = GetLastError();
                printf("send error (%d), %s\n", err, strerror(err));
                break;
            } else if (sz == 0) {
                int err = GetLastError();
                printf("send error (%d), %s\n", err, strerror(err));
                break;
            }
            left = left - sz;
            printf("send success (%d)\n", sz);
        }

        sz = recv(sock, buf, BUF_LEN, flags);
        if (sz == -1) {
            int err = GetLastError();
            printf("recv error (%d), %s\n", err, strerror(err));
            break;
        } else if (sz == 0) {
            int err = GetLastError();
            printf("recv error (%d), %s\n", err, strerror(err));
            break;
        }
        printf("recv success (%d)\n", sz);

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
