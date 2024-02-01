#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE // reallocarray()
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h> // for fd_set
#include <assert.h>
#include <stdio.h>
#include <string.h> // memset
#include <arpa/inet.h> // inet_pton

#include <ares.h>

// used by `getent hosts`
struct hostent *gethostbyname2(const char *name, int af)
{
    exit(66);
}

static void addrinfo_from_ares(struct addrinfo *libc_addrinfo, struct ares_addrinfo_node *node)
{
    libc_addrinfo->ai_flags = node->ai_flags;
    libc_addrinfo->ai_family = node->ai_family;
    libc_addrinfo->ai_socktype = node->ai_socktype;
    libc_addrinfo->ai_protocol = node->ai_protocol;
    libc_addrinfo->ai_addrlen = node->ai_addrlen;
    libc_addrinfo->ai_addr = malloc(libc_addrinfo->ai_addrlen);
    memcpy(libc_addrinfo->ai_addr, node->ai_addr, libc_addrinfo->ai_addrlen);
    libc_addrinfo->ai_canonname = NULL; // c-ares doesn't provide it
}

static void ai_callback(void *arg, int status, int timeouts,
                        struct ares_addrinfo *result)
{
    struct ares_addrinfo_node *node = NULL;
    (void)timeouts;
    struct addrinfo **restrict libc_res = arg;
    size_t libc_res_nmemb = 0;
    int next_memb = 0;
    *libc_res = NULL;



    if (status != ARES_SUCCESS) {
        if (getenv("DEBUG")) {
            fprintf(stderr, __FILE__ ": %s: %s\n", (char *)arg, ares_strerror(status));
        }
        return;
    }

    for (node = result->nodes; node != NULL; node = node->ai_next) {
        char        addr_buf[64] = "";
        const void *ptr          = NULL;

        if (libc_res_nmemb <= next_memb) {
            libc_res_nmemb += 1;
            *libc_res = reallocarray(*libc_res, libc_res_nmemb, sizeof(struct addrinfo));
        }
        struct addrinfo *libc_addrinfo = &((*libc_res)[next_memb]);
        next_memb += 1;

        if (node->ai_family == AF_INET) {
            const struct sockaddr_in *in_addr =
                (const struct sockaddr_in *)((void *)node->ai_addr);
            ptr = &in_addr->sin_addr;
            addrinfo_from_ares(libc_addrinfo, node);
        } else if (node->ai_family == AF_INET6) {
            const struct sockaddr_in6 *in_addr =
                (const struct sockaddr_in6 *)((void *)node->ai_addr);
            ptr = &in_addr->sin6_addr;
            addrinfo_from_ares(libc_addrinfo, node);
        } else {
            continue;
        }
        ares_inet_ntop(node->ai_family, ptr, addr_buf, sizeof(addr_buf));
        if (getenv("DEBUG")) {
            printf(__FILE__ ": %-32s\t%s\n", result->name, addr_buf);
        }
    }

    // Link ai_next. Do it late because we reallocate as we read the results.
    for (int i = 0; i < next_memb; i++) {
        struct addrinfo *cur  = &((*libc_res)[i  ]);
        struct addrinfo *next = &((*libc_res)[i+1]);
        if (i == next_memb - 1) {
            cur->ai_next = NULL;
        } else {
            cur->ai_next = next;
        }
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
    const char *name_override = getenv("NAME_OVERRIDE");
    if (name_override && name_override[0] != '\0') {
        libc_node = name_override;
    }
    const char* local_dev_name = getenv("LOCAL_DEV");
    if (local_dev_name) {
        // requires root privileges, failure to apply is silently ignored
        ares_set_local_dev(channel, local_dev_name);
    }
    const char *local_ip4 = getenv("LOCAL_IP4");
    if (local_ip4) {
        struct in_addr binary_address_ip4 = {0};
        r = inet_pton(AF_INET, local_ip4, &binary_address_ip4);
        if (r == 1) {
            ares_set_local_ip4(channel, ntohl(binary_address_ip4.s_addr));
        } else {
            if (getenv("DEBUG")) {
                fprintf(stderr, __FILE__ ": inet_pton(%s) failed\n", local_ip4);
            }
        }
    }
    const char *local_ip6 = getenv("LOCAL_IP6");
    if (local_ip6) {
        struct in6_addr binary_address_ip6 = {0};
        r = inet_pton(AF_INET6, local_ip6, &binary_address_ip6);
        if (r == 1) {
            ares_set_local_ip6(channel, (const unsigned char*)&binary_address_ip6);
        } else {
            if (getenv("DEBUG")) {
                fprintf(stderr, __FILE__ ": inet_pton(%s) failed\n", local_ip4);
            }
        }
    }
    const char *servers_csv = getenv("SERVERS_CSV");
    if (servers_csv) {
        r = ares_set_servers_csv(channel, servers_csv);
        if (r != ARES_SUCCESS) {
            if (getenv("DEBUG")) {
                fprintf(stderr, __FILE__ ": ares_set_servers_csv(%s) failed: %d\n", servers_csv, r);
            }
        }
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

    ares_destroy(channel);
    ares_library_cleanup();
    return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
    struct addrinfo *entry = res;
    while (entry) {
        free(entry->ai_addr);
        entry = entry->ai_next;
    }
    free(res);
}
