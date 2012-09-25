#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "geo.h"

#define MAX_LINE    1024

/* "\t\n\r " */
static const unsigned char default_ifs[256] = 
    { [9]=1, [10]=1, [13]=1, [32]=1 };

static int str_explode(const unsigned char *ifs, unsigned char *buf, 
        unsigned char *field[], int n) {
    int i = 0;
    unsigned char *tempifs;

    /* When ifs is NULL, use the default blanks. If the first
       byte is NULL, use the IFS table, otherwise, use the IFS
       array as a separator table. */
    if (ifs == NULL) {
        ifs = default_ifs;
    } else if (*ifs) {
        tempifs = (unsigned char *)alloca(256);
        memset((void*)tempifs, 0, 256);
        while (*ifs) {
            tempifs[*ifs++] = 1;
        }
        ifs = tempifs;
    } 

    i = 0;
    while (1) {
        /* Trim the leading separators */
        while (ifs[*buf]) {
            buf++;
        }

        if (!*buf) { 
            break;
        }

        field[i++] = buf;

        if (i >= n) { /* Process the last field. */
            buf += strlen((char *)buf) - 1;
            while (ifs[*buf]) {
                --buf;
            }
            *(buf + 1) = '\0';
            break;
        }

        while (*buf && !ifs[*buf]) {
            ++buf;
        }

        if (!*buf) {
            break;
        }
        *buf++ = '\0';
    }
    return i;
}

static int parse_range(char *range, uint32_t *start, uint32_t *end) {
    char            *p;
    struct in_addr  in;

    p = strchr(range, '-');
    if (!p) {
        return -1;
    }

    *p = '\0';
    ++p;
    if (inet_aton(range, &in) == 0) {
        return -1;
    }
    *start = ntohl(in.s_addr);

    if (inet_aton(p, &in) == 0) {
        return -1;
    }
    *end = ntohl(in.s_addr);
    return 0;
}

static int geo_parse_range(geo_t *g, char *range, char *value) {
    uint32_t ip_addr;
    struct in_addr in;
    uint32_t start, end;

    if (strcmp(range, "default") == 0) {
        if (inet_aton(value, &in) == 0) {
            return -1;
        }
        ip_addr = ntohl(in.s_addr);
        range_set_def(g->u.range, ip_addr);
    } else if (strcmp(range, "delete") == 0) {
        if (parse_range(value, &start, &end) != 0) {
            return -1;
        }
        range_delete(g->u.range, start, end);
        return 0;
    } else {
        if (inet_aton(value, &in) == 0) {
            return -1;
        }
        ip_addr = ntohl(in.s_addr);

        if (parse_range(range, &start, &end) != 0) {
            return -1;
        }

        return range_insert(g->u.range, start, end, ip_addr);
    }
    return 0;
}

static int parse_cidr(char *cidr, uint32_t *addr, uint32_t *mask) {
    char *p;
    uint32_t shift;
    struct in_addr in;

    if (strcmp(cidr, "255.255.255.255") == 0) {
        *addr = 0xffffffff;
        *mask = 0xffffffff;
        return 0;
    }

    p = strchr(cidr, '/');
    if (!p) {
        if (inet_aton(cidr, &in) == 0) {
            return -1;
        }
        *addr = ntohl(in.s_addr);
        *mask = 0;
        return 0;
    } else {
        *p++ = '\0';
        if (inet_aton(cidr, &in) == 0) {
            return -1;
        }
        *addr = ntohl(in.s_addr);
        shift = atoi(p);
        if (shift > 32) {
            return -1;
        }

        if (shift) {
            *mask = (uint32_t)(0xffffffffu << (32 - shift));
        } else {
            *mask = 0;
        }

        return 0;
    }
    return -1;
}

static int geo_parse_cidr(geo_t *g, char *cidr, char *value) {
    struct in_addr  in;
    int             i;
    uint32_t        addr;
    uint32_t        mask;
    uint32_t        ip_addr;
    uint32_t        old;

    if (strcmp(cidr, "default") == 0) {
        if (inet_aton(value, &in) == 0) {
            return -1;
        }
        ip_addr = ntohl(in.s_addr);
        addr = 0;
        mask = 0;
    } else if (strcmp(cidr, "delete") == 0) {
        if (parse_cidr(value, &addr, &mask) != 0) {
            return -1;
        }
        if (radix32tree_delete(g->u.tree, addr, mask) != 0) {
            return -1;
        }
        return 0;
    } 
 
    if (parse_cidr(cidr, &addr, &mask) != 0) {
        return -1;
    }

    if (inet_aton(value, &in) == 0) {
        return -1;
    }
    ip_addr = ntohl(in.s_addr);

    /* Retry 3 times */
    for (i = 3; i; --i) {
        if (radix32tree_insert(g->u.tree, addr, mask, ip_addr) == 0) {
            return 0;
        }

        old = radix32tree_find(g->u.tree, addr & mask);
        /* TODO log */
        if (radix32tree_delete(g->u.tree, addr, mask) != 0) {
            return -1;
        }
    }

    return -1;
}

geo_t *geo_load(geo_t *g, char *filename, int geo_mode) {
    int             n;
    FILE            *fp;
    char            buf[MAX_LINE];
    char            *p;
    unsigned char   *field[2];

    if (!filename) {
        return NULL;
    }

    if (geo_mode != GEO_RANGE && geo_mode != GEO_CIDR) {
        return NULL;
    }

    if (!g) {
        g = (geo_t *)malloc(sizeof(geo_t));
        if (!g) {
            return NULL;
        }

        g->geo_mode = geo_mode;
        if (geo_mode == GEO_RANGE) {
            g->u.range = range_create();
            if (!g->u.range) {
                free(g);
                return NULL;
            }
        } else if (geo_mode == GEO_CIDR) {
            g->u.tree = radix_tree_create();
            if (!g->u.tree) {
                free(g);
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }

    while (fgets(buf, MAX_LINE, fp)) {
        n = strlen(buf);
        if (buf[n - 1] == '\n') {
            buf[n - 1] = '\0';
        }

        p = buf;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }

        if (*p == '\0' || *p == '#' || (*p == '/' && *(p + 1) == '/')) {
            continue;
        }

        if (str_explode(NULL, (unsigned char *)p, field, 2) != 2) {
            goto error;
        }

        if (strcmp((char *)field[0], "include") == 0) {
            if (geo_load(g, (char *)field[1], geo_mode) == NULL) {
                goto error;
            }
            continue;
        }

        if (geo_mode == GEO_RANGE) {
            if (geo_parse_range(g, (char *)field[0], (char *)field[1]) != 0) {
                goto error;
            }
        } else if (geo_mode == GEO_CIDR) {
            if (geo_parse_cidr(g, (char *)field[0], (char *)field[1]) != 0) {
                goto error;
            }
        } else {
            /* should NOT get here */
            assert(0);
        }
    }

    fclose(fp);
    return g;

error:
    fclose(fp);
    geo_unload(g);
    return NULL;
}

uint32_t geo_get(geo_t *g, uint32_t ip) {
    uint32_t ret = INVALID_IP_ADDR;

    if (g->geo_mode == GEO_RANGE) {
        return range_get(g->u.range, ip);
    } else if (g->geo_mode == GEO_CIDR) {
        if ((ret = radix32tree_find(g->u.tree, ip)) == RADIX_NO_VALUE) {
            return INVALID_IP_ADDR;
        }
        return ret;
    } else {
        assert(0);
    }
    return ret;
}

char *geo_get_str(geo_t *g, char *ip_str) {
    struct in_addr  in;
    uint32_t        ip_addr;

    if (inet_aton(ip_str, &in) == 0) {
        return NULL;
    }
    ip_addr = ntohl(in.s_addr);
    ip_addr = geo_get(g, ip_addr);
    if (ip_addr == INVALID_IP_ADDR) {
        return NULL;
    }
    in.s_addr = htonl(ip_addr);
    return inet_ntoa(in);
}

void geo_unload(geo_t *g) {
    if (g->geo_mode == GEO_RANGE) {
        range_free(g->u.range);
    } else if (g->geo_mode == GEO_CIDR) {
        radix_tree_free(g->u.tree);
    } else {
        /* should NOT get here */
        assert(0);
    }
    free(g);
}

void geo_dump(geo_t *g, int geo_mode) {
    if (geo_mode == GEO_RANGE) {
        range_dump(g->u.range);
    } else if (geo_mode == GEO_CIDR) {
        printf("Not supported\n");
    }
}

/* gcc -D GEO_TEST_MAIN geo.c range.c radix.c vector.c */
#ifdef GEO_TEST_MAIN
int main(int argc, char **argv) {
    char *ip_str;
    geo_t *g = geo_load(NULL, "range.geo", GEO_RANGE);
    assert(g);
    geo_dump(g, GEO_RANGE);

    printf("*********************************\n");
    ip_str = "192.168.100.11";
    printf("%s => %s\n", ip_str, geo_get_str(g, ip_str));
    
    ip_str = "172.23.9.43";
    printf("%s => %s\n", ip_str, geo_get_str(g, ip_str));
    geo_unload(g);

    printf("=====================================\n");
    g = geo_load(NULL, "cidr.geo", GEO_CIDR);
    assert(g);

    printf("*************************************\n");
    ip_str = "172.23.0.111";
    printf("%s => %s\n", ip_str, geo_get_str(g, ip_str));

    ip_str = "192.168.100.1";
    printf("%s => %s\n", ip_str, geo_get_str(g, ip_str));

    ip_str = "202.118.80.2";
    printf("%s => %s\n", ip_str, geo_get_str(g, ip_str));
    geo_unload(g);
    exit(0);
}
#endif /* GEO_TEST_MAIN */
