#include "dns.h"

int dns_parse_proxy(unsigned char *pkt, int len) {
    unsigned char *p, *s, *e;
    dns_header_t *header;
    int found, stop, dlen, nlen;

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

    }
}
