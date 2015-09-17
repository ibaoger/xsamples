#include <sys/types.h>
#include <sys/socket.h>

int serverPort = 32015;

int main(int argc, char **argv)
{
    int err;
    int sock;
    struct sockaddr_in serveraddr;

    /* get a socket descriptor */
    int domain;
    int type;
    int protocol;
    err = socket(AF_INET, SOCK_STREAM, int protocol);

    return 0;
}
