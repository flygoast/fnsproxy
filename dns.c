#include <stdio.h>
#include "srv.h"
#include "dns.h"
#include "log.h"

static int process_ip(unsigned char *p, int len) {
    uint32_t ip;
    uint32_t proxy;
    struct in_addr in;
    char    buf_in[INET_ADDRSTRLEN + 1];
    char    buf_proxy[INET_ADDRSTRLEN + 1];

    if (len != 4) {
        return -1;
    }

    ip = ntohl(*(uint32_t *)p);
    if ((proxy = geo_get(fnsproxy_srv.geo, ip)) != INVALID_IP_ADDR) {
        in.s_addr = *(uint32_t *)p;
        snprintf(buf_in, sizeof(buf_in), inet_ntoa(in));
        proxy = htonl(proxy);
        *(uint32_t *)p = proxy;
        in.s_addr = proxy;
        snprintf(buf_proxy, sizeof(buf_proxy), inet_ntoa(in));
        DEBUG_LOG("Replace IP %s with %s", buf_in, buf_proxy);
        return 0;
    }
    return -1;
}

int dns_parse_proxy(unsigned char *pkt, int len) {
    unsigned char *p, *s, *e;
    dns_header_t *header;
    int found, stop, dlen, nlen;
    uint16_t type;

    header = (dns_header_t *)pkt;
    if (ntohs(header->nquestions) != 1) {
        return -1;
    }

    /* skip hostname */
    for (e = pkt + len, nlen = 0, s = p = &header->data[0];
            p < e && *p != '\0'; ++p) {
        ++nlen;
    }

    ++p;
    if (&p[4] > e || ntohs(*(uint16_t *)p) != DNS_A_RECORD) {
        return -1;
    }

    /* seek to the first answer */
    p += 4;

    /* loop through the answers, we want A record */
    for (found = stop = 0; !stop && &p[12] < e; ) {
        /* skip possible name in CNAME answer */
        if ((*p & 0xc0) != 0xc0) {
            while (*p && &p[12] < e) {
                ++p;
            }
            --p;
        }

        type = ntohs(*((uint16_t *)p + 1));
        if (type == DNS_CNAME_RECORD) {
            /* CNAME answer. shift to the next section */
            dlen = ntohs(*((uint16_t *)p + 5));
            p += 12 + dlen;
        } else if (type == DNS_A_RECORD) {
            found = stop = 1;
        } else {
            stop = 1;
        }
    }

    if (found && p + 12 < e) {
        dlen = ntohs(*((uint16_t *)p + 5));
        p += 12;

        if (p + dlen <= e) {
            return process_ip(p, dlen);
        }
    }
    return 0;
}
