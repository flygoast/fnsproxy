#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "vector.h"
#include "range.h"

#define RANGE_INIT_NUM      16

range_t *range_create() {
    range_t *r = (range_t *)malloc(sizeof(*r));
    if (!r) {
        return NULL;
    }

    if (range_init(r) != 0) {
        free(r);
        return NULL;
    }
    return r;
}

int range_init(range_t *r) {
    r->def_ip = INVALID_IP_ADDR;
    r->low = (vector_t **)calloc(0x10000, sizeof(vector_t *));
    if (!r->low) {
        return -1;
    }
    return 0;
}

int range_insert(range_t *r, uint32_t start, uint32_t end, uint32_t ip_addr) {
    uint32_t    n;
    uint32_t    h, i, s, e;
    vector_t    *vec;
    range_low_t *low;
    range_low_t rl, invalid;

    invalid.ip_addr = INVALID_IP_ADDR;
    invalid.start = 0;
    invalid.end = 0;

    if (start > end) {
        return -1;
    }

    for (n = start; n <= end; n = (n + 0x10000) & 0xffff0000) {
        h = n >> 16;
        if (n == start) {
            s = n & 0xffff;
        } else {
            s = 0;
        }

        if ((n | 0xffff) > end) {
            e = end & 0xffff;
        } else {
            e = 0xffff;
        }

        vec = r->low[h];

        if (!vec) {
            vec = vector_new(RANGE_INIT_NUM, sizeof(range_low_t));
            if (!vec) {
                return -1;
            }
            r->low[h] = vec;
        }

        i = vec->count;
        low = (range_low_t *)vec->data;
        while (i) {
            --i;
            if (e < low[i].start) {
                continue;
            }

            if (s > low[i].end) {
                /* add after the range */
                if (vector_push(vec, &invalid) < 0) {
                    return -1;
                }

                low = (range_low_t *)vec->data;
                memmove(&low[i + 2], &low[i + 1],
                        (vec->count - 2 - i) * sizeof(range_low_t));
                low[i + 1].ip_addr = ip_addr;
                low[i + 1].start = s;
                low[i + 1].end = e;

                goto next;
            }

            if (s == low[i].start && e == low[i].end) {
                /* TODO log */
                low[i].ip_addr = ip_addr;
                goto next;
            }

            if (s > low[i].start && e < low[i].end) {
                /* split the range and insert the new one */
                if (vector_push(vec, &invalid) < 0) {
                    return -1;
                }

                if (vector_push(vec, &invalid) < 0) {
                    return -1;
                }

                low = (range_low_t *)vec->data;
                memmove(&low[i + 3], &low[i + 1],
                        (vec->count - 3 - i) * sizeof(range_low_t));

                low[i + 2].start = (uint16_t)(e + 1);
                low[i + 2].end = low[i].end;
                low[i + 2].ip_addr = low[i].ip_addr;

                low[i + 1].start = s;
                low[i + 1].end = e;
                low[i + 1].ip_addr = ip_addr;

                low[i].end = (uint16_t)(s - 1);
                goto next;
            }

            if (s == low[i].start && e < low[i].end) {
                /* shift the range start and insert the new range */
                if (vector_push(vec, &invalid) < 0) {
                    return -1;
                }

                low = (range_low_t *)vec->data;
                memmove(&low[i + 1], &low[i],
                        (vec->count - 1 - i) * sizeof(range_low_t));

                low[i + 1].start = (uint16_t)(e + 1);

                low[i].start = s;
                low[i].end = e;
                low[i].ip_addr = ip_addr;
                goto next;
            }

            if (s > low[i].start && e == low[i].end) {
                /* shift the range end and insert the new range */
                if (vector_push(vec, &invalid) < 0) {
                    return -1;
                }

                low = (range_low_t *)vec->data;
                memmove(&low[i + 2], &low[i + 1],
                        (vec->count - 2 - i) * sizeof(range_low_t));
                low[i + 1].start = s;
                low[i + 1].end = e;
                low[i + 1].ip_addr = ip_addr;

                low[i].end = (uint16_t)(s - 1);
                goto next;
            }

            s = low[i].start;
            e = low[i].end;

            /* TODO log */
            return -1;
        }

        /* add the first range */
        rl.ip_addr = ip_addr;
        rl.start = s;
        rl.end = e;

        if (vector_push(vec, &rl) < 0) {
            return -1;
        }
next:
        continue;
    }
    return 0;
}

int range_insert_str(range_t *r, char *start, char *end, char *ip) {
    uint32_t s, e, ip_addr;
    struct in_addr in;

    if (inet_aton(start, &in) == 0) {
        return -1;
    }
    s = ntohl(in.s_addr);

    if (inet_aton(end, &in) == 0) {
        return -1;
    }
    e = ntohl(in.s_addr);

    if (inet_aton(ip, &in) == 0) {
        return -1;
    }
    ip_addr = ntohl(in.s_addr);

    return range_insert(r, s, e, ip_addr);
}

void range_delete(range_t *r, uint32_t start, uint32_t end) {
    uint32_t    n;
    uint32_t    h, i, s, e;
    vector_t    *vec;
    range_low_t *low;

    for (n = start; n <= end; n += 0x10000) {
        h = n >> 16;
        if (n == start) {
            s = n & 0xffff;
        } else {
            s = 0;
        }

        if ((n | 0xffff) > end) {
            e = end & 0xffff;
        } else {
            e = 0xffff;
        }

        vec = r->low[h];

        if (vec == NULL) {
            continue;
        }

        low = (range_low_t *)vec->data;
        for (i = 0; i < vec->count; ++i) {
            if (s == low[i].start && e == low[i].end) {
                memmove(&low[i], &low[i + 1],
                        (vec->count - 1 - i) * sizeof(range_low_t));
                --vec->count;
                break;
            }

            if (s != low[i].start && e != low[i].end) {
                continue;
            }

            /* TODO log */
        }
    }
}

void range_delete_str(range_t *r, char *start, char *end) {
    uint32_t s, e;
    struct in_addr in;

    if (inet_aton(start, &in) == 0) {
        return;
    }
    s = ntohl(in.s_addr);

    if (inet_aton(end, &in) == 0) {
        return;
    }
    e = ntohl(in.s_addr);

    range_delete(r, s, e);
}

uint32_t range_get(range_t *r, uint32_t key) {
    uint32_t        ip_addr;
    uint32_t        n;
    int             i;
    vector_t        *vec;
    range_low_t     *low;

    ip_addr = r->def_ip;
    vec = r->low[key >> 16];
    if (vec) {
        low = (range_low_t*)vec->data;
        n = key & 0xffff;

        for (i = 0; i < vec->count; ++i) {
            if (n >= low[i].start && n <= low[i].end) {
                ip_addr = low[i].ip_addr;
                break;
            }
        }
    }
    return ip_addr;
}

char *range_get_str(range_t *r, char *key) {
    uint32_t k, v;
    struct in_addr in;

    if (inet_aton(key, &in) == 0) {
        return NULL;
    }
    k = ntohl(in.s_addr);
    v = range_get(r, k);

    in.s_addr = htonl(v);
    return inet_ntoa(in);
}

void range_destroy(range_t *r) {
    int i;
    for (i = 0; i < 0x10000; ++i) {
        if (r->low[i]) {
            vector_free(r->low[i]);
            r->low[i] = NULL;
        }
    }
    free(r->low);
}

void range_free(range_t *r) {
    range_destroy(r);
    free(r);
}

/* gcc vector.c range.c -D RANGE_TEST_MAIN */
#ifdef RANGE_TEST_MAIN
#include <assert.h>

void range_dump(range_t *r) {
    int             i, j;
    char            *addr_s;
    char            *addr_e;
    char            *def_ip;
    vector_t        *vec;
    range_low_t     *low;
    struct in_addr  in;

    in.s_addr = htonl(r->def_ip);
    def_ip = inet_ntoa(in);

    printf("%-16s %-16s %s\n", "start", "end", "ip addr");
    printf("=============================================\n");
    printf("%-33s %s\n", "default", def_ip);

    for (i = 0; i < 0x10000; ++i) {
        vec = r->low[i];
        if (!vec) {
            continue;
        }
        low = (range_low_t *)vec->data;
        for (j = 0; j < vec->count; ++j) {
            in.s_addr = htonl((i << 16) | low[j].start);
            addr_s = inet_ntoa(in);
            printf("%-16s ", addr_s);
            in.s_addr = htonl((i << 16) | low[j].end);
            addr_e = inet_ntoa(in);
            printf("%-16s ", addr_e);
            in.s_addr = htonl(low[j].ip_addr);
            printf("%s\n", inet_ntoa(in));
        }
    }
}

int main(int argc, char **argv) {
    char *start, *end, *ip_addr, *ip;

    range_t *r = range_create();
    range_set_def(r, 0xffffffff);
    assert(r);
    start = "192.168.0.1";
    end = "192.168.0.100";
    ip_addr = "127.0.0.1";
    
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.10";
    end = "192.168.0.50";
    ip_addr = "127.0.0.2";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "172.23.0.11";
    end = "172.24.0.50";
    ip_addr = "127.0.0.3";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.120";
    end = "192.168.0.150";
    ip_addr = "127.0.0.4";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.110";
    end = "192.168.0.115";
    ip_addr = "127.0.0.5";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.110";
    end = "192.168.0.115";
    ip_addr = "127.0.0.6";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.110";
    end = "192.168.0.112";
    ip_addr = "127.0.0.7";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    start = "192.168.0.110";
    end = "192.168.0.112";
    ip_addr = "127.0.0.7";
    assert(range_insert_str(r, start, end, ip_addr) == 0);

    /* overlaps */
    start = "192.168.0.112";
    end = "192.168.0.114";
    ip_addr = "127.0.0.8";
    assert(range_insert_str(r, start, end, ip_addr) < 0);

    start = "192.168.0.113";
    end = "192.168.0.115";
    range_delete_str(r, start, end);

    start = "192.168.0.120";
    end = "192.168.0.150";
    range_delete_str(r, start, end);

    range_dump(r);
    printf("**********************************\n");
    
    ip = "192.168.0.123";
    printf("%s => %s\n", ip, range_get_str(r, ip));

    ip = "192.168.0.111";
    printf("%s => %s\n", ip, range_get_str(r, ip));

    ip = "192.168.0.51";
    printf("%s => %s\n", ip, range_get_str(r, ip));

    ip = "192.168.0.50";
    printf("%s => %s\n", ip, range_get_str(r, ip));

    ip = "172.23.111.50";
    printf("%s => %s\n", ip, range_get_str(r, ip));

    range_free(r);
    exit(0);
}

#endif /* RANGE_TEST_MAIN */
