#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


// used by `getent hosts`
struct hostent *gethostbyname2(const char *name, int af)
{
    exit(66);
}

// used by: nslookup, ping, dig
int getaddrinfo(const char *restrict node,
                const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res)
{
    exit(77);
}

void freeaddrinfo(struct addrinfo *res)
{
    exit(77);
}
