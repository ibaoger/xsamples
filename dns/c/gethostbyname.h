#ifndef _GETHOSTBYNAME_H_
#define _GETHOSTBYNAME_H_


#define MAX_IP_LEN 24
struct IP
{
    char ip[MAX_IP_LEN];
    unsigned short len;
};

int GetHostByName(const char *name, struct IP *ip);
void ForceCloseGetHostByName();


#endif /*_GETHOSTBYNAME_H_*/