#ifdef _WIN32
#else
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#endif //_WIN32 && unix
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"


char time_string[16] = { 0 };

#ifdef _WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    if (tv == NULL)
        return -1;

    DWORD tick = GetTickCount();
    tv->tv_sec = (long)(tick / 1000);
    tv->tv_usec = (long)(tick % 1000) * 1000;
    return 0;
}
#else
int closesocket(int fd)
{
    return close(fd);
}
#endif /*_WIN32*/

char* get_time_string() {
    /* get ms time */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int tv_ms = tv.tv_usec / 1000;
    memset(time_string, 0, sizeof(time_string));
    sprintf(time_string, "%s.%d", __TIME__, tv_ms);
    return (char*)&time_string;
}

int get_socket_error()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

void set_socket_error(int err)
{
#ifdef _WIN32
    WSASetLastError(err);
#else
    errno = err;
#endif
}

/* UNIX */
/* connect: readable or writeable, and no socket error */
/* send: writeable, and no socket error */
/* recv: readable, and no socket error */
/* WIN32 */
/* connect: readable or writeable, and no select error */
/* send: writeable, and no select error */
/* recv: readable, and no select error */
/*  0: ready */
/* -1: error */
/* -2: time out */
int wait_for_ready(int s, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *timeo)
{
    int rc = select(s + 1, rd, wr, ex, timeo);
    int err = get_socket_error();
    if (rc < 0) {
        printf("%s select error (%d), %s\n", get_time_string(), err, strerror(err));
        return -1;
    } else if (rc == 0) {
        printf("%s select time out (%d), %s\n", get_time_string(), err, strerror(err));
        return -2;
    } else {
        bool ready = true;
        if (rd && wr) {
            ready = ready && (FD_ISSET(s, rd) || FD_ISSET(s, wr));
        } else if (rd) {
            ready = ready && FD_ISSET(s, rd);
        } else if (wr) {
            ready = ready && FD_ISSET(s, wr);
        }

        if (ex) {
#ifdef _WIN32
            ready = ready && !FD_ISSET(s, ex);
#else
            int err;
            unsigned int len = sizeof(err);
            rc = getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len);
            ready = ready && (rc >= 0 && err == 0);
            set_socket_error(err);
#endif
        }

        if (ready)
            return 0;

        printf("%s select error (%d), %s\n", get_time_string(), err, strerror(err));
        return -1;
    }
    return -1;
}
