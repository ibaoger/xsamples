
struct DNS_PACKAGE {
    char *data;
    unsigned int datalen;
}, dnsPackage;

struct DNS_HEADER {
    unsigned short id;
    unsigned short flag;
    unsigned short questions;
    unsigned short resources;
    unsigned short authors;
    unsigned short exts;
}, dnsHeader;

struct DNS_QUESTION {
    char *name;
    unsigned short type;
    unsigned short clas;
}, dnsQuestion;

struct DNS_RESOURCE {
    unsigned short name;
    unsigned short type;
    unsigned short clas;
    unsigned int ttl;
    unsigned short datalen;
    char *data;
}, dnsResource;

#define MAX_UDP_MSG_SIZE 512
const char defaultAQueryHeader[12] = {0x43, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char dnsSocketBuf[MAX_UDP_MSG_SIZE] = {0};
const struct timeval defaultTimeout = {30, 0};

struct hostent* GetHostByName(const char *name)
{

}

dnsQuestion pack_query_question(const char *name)
{

}

int wait_for_ready(int s, fd_set rd, fd_set wr, fd_set ex)
{

}

unpack_response_resource()
{

}
