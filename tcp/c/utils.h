/***************************************************************

 * 模  块：xsamples
 * 文  件：utils.h
 * 功  能：网络示例程序的辅助功能。
 * 作  者：阿宝（Po）
 * 日  期：2015-09-27
 * 版  权：Copyright (c) 2012-2015 Dream Company

 ***************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <time.h>

#ifdef _WIN32
/* same as linux gettimeofday */
int gettimeofday(struct timeval *tv, struct timezone *tz);
#else
int closesocket(int fd);
#endif /*_WIN32*/

/* get time string, as HH:MM:SS.mmm */
char* get_time_string();

/* get/set socket error */
int get_socket_error();
void set_socket_error(int err);

/* select socket is ready ? */
int wait_for_ready(int s, fd_set *rd, fd_set *wr, fd_set *ex, struct timeval *timeo);

#endif /*_UTILS_H_*/
