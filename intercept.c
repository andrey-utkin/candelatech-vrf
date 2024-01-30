#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h> // for fd_set
#include <assert.h>
#include <stdio.h>
#include <string.h> // memset

#include <ares.h>

// used by `getent hosts`
struct hostent *gethostbyname2(const char *name, int af)
{
    exit(66);
}

static void z(struct addrinfo *libc_addrinfo, struct ares_addrinfo_node *node)
{
    // casted struct copying, replace with per-field copying later
    *libc_addrinfo = *(struct addrinfo*)node;
    libc_addrinfo->ai_flags = node->ai_flags;
    libc_addrinfo->ai_family = node->ai_family;
    libc_addrinfo->ai_socktype = node->ai_socktype;
    libc_addrinfo->ai_protocol = node->ai_protocol;
    libc_addrinfo->ai_addrlen = node->ai_addrlen;

    libc_addrinfo->ai_next = NULL;
    libc_addrinfo->ai_addr = calloc(1, libc_addrinfo->ai_addrlen);
    memcpy(libc_addrinfo->ai_addr, node->ai_addr, libc_addrinfo->ai_addrlen);
}

static void ai_callback(void *arg, int status, int timeouts,
                        struct ares_addrinfo *result)
{
  struct ares_addrinfo_node *node = NULL;
  (void)timeouts;
  struct addrinfo **restrict libc_res = arg;
  *libc_res = calloc(1, sizeof(struct addrinfo));
  struct addrinfo *libc_addrinfo = libc_res[0];



  if (status != ARES_SUCCESS) {
    fprintf(stderr, "%s: %s\n", (char *)arg, ares_strerror(status));
    return;
  }

  for (node = result->nodes; node != NULL; node = node->ai_next) {
    char        addr_buf[64] = "";
    const void *ptr          = NULL;

    if (node->ai_family == AF_INET) {
      const struct sockaddr_in *in_addr =
        (const struct sockaddr_in *)((void *)node->ai_addr);
      ptr = &in_addr->sin_addr;
      z(libc_addrinfo, node);
    } else if (node->ai_family == AF_INET6) {
      const struct sockaddr_in6 *in_addr =
        (const struct sockaddr_in6 *)((void *)node->ai_addr);
      ptr = &in_addr->sin6_addr;
      z(libc_addrinfo, node);
    } else {
      continue;
    }
    ares_inet_ntop(node->ai_family, ptr, addr_buf, sizeof(addr_buf));
    printf(__FILE__ ": %-32s\t%s\n", result->name, addr_buf);
  }

  ares_freeaddrinfo(result);
}

// used by: nslookup, ping, dig
int getaddrinfo(const char *restrict libc_node,
                const char *restrict libc_service,
                const struct addrinfo *restrict libc_hints,
                struct addrinfo **restrict libc_res)
{
    int r;
    r = ares_library_init(ARES_LIB_INIT_ALL);
    assert(!r);
    ares_channel_t *channel = NULL;
    struct ares_options options = {0};
    r = ares_init_options(&channel, &options, /*optmask=*/0);
    assert(!r);
    struct ares_addrinfo_hints hints;
    memset(&hints, 0, sizeof(hints));
    if (libc_hints) {
        hints.ai_flags = libc_hints->ai_flags;
        hints.ai_family = libc_hints->ai_family;
        hints.ai_socktype = libc_hints->ai_socktype;
        hints.ai_protocol = libc_hints->ai_protocol;
    }
    ares_getaddrinfo(channel, libc_node, libc_service, &hints, ai_callback, /*arg=*/libc_res);

    // wait for the query to be completed...
    // fill back the output parameters...

    // put this loop somewhere outside, or change its condition to terminate when the query has completed
    // ares_fds(3)
    int nfds, count;
    fd_set readers, writers;
    struct timeval tv, *tvp;
    while (1) {
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        nfds = ares_fds(channel, &readers, &writers);
        if (nfds == 0)
            break;
        tvp = ares_timeout(channel, NULL, &tv);
        count = select(nfds, &readers, &writers, NULL, tvp);
        ares_process(channel, &readers, &writers);
    }

    ares_library_cleanup();
    //exit(77);
    return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
    //exit(77);
    free(res);
}
