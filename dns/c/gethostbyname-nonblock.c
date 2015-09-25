/***************************************************************

 * 模  块：xsamples
 * 文  件：gethostbyname-nonblock.c
 * 功  能：DNS解析，非阻塞，超时
 * 作  者：阿宝（Po）
 * 日  期：2015-09-22
 * 版  权：Copyright (c) 2012-2014 Dream Company

***************************************************************/

#ifdef _WIN32
#include <Windows.h>
#else
#endif /* _WIN32 */
#include <stdlib.h>
#include <stdio.h>
#include "gethostbyname.h"

/* load library */

/* macro define */

/* pre declare */

/* member value */

int main(int argc, char **argv)
{
    struct IP ip;
    const char *name = "live.heysound.com";
    if (GetHostByName(name, &ip) == 0)
        printf("success: %s\n", ip.ip);
    else
        printf("error\n");
    ForceCloseGetHostByName();
    getchar();
	return 0;
}
