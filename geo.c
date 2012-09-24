#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    *p = '0';
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

static int geo_parse_range(geo_t *g, unsigned char *range, 
        unsigned char *value) {
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
        if (parse_range(range, &start, &end) != 0) {
            return -1;
        }

        return range_insert(g->u.range, start, end, ip_addr);
    }

    return 0;
}

geo_t *geo_load(char *filename, int geo_mode) {
    FILE            *fp;
    char            buf[MAX_LINE];
    char            *p;
    unsigned char   *field[2];
    geo_t           *g;


    if (!filename) {
        return NULL;
    }

    if (geo_mode != GEO_RANGE && geo_mode != GEO_CIDR) {
        return NULL;
    }

    fp = fopen(filename, "r");
    if (fp) {
        return NULL;
    }

    g = (geo_t *)malloc(sizeof(g));
    if (!g) {
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

        if (*p == '#' || (*p == '/' && *(p + 1) == '/')) {
            continue;
        }

        if (str_explode(NULL, (unsigned char *)p, field, 2) != 2) {
            goto error;
        }

        if (geo_mode == GEO_RANGE) {
            if (geo_parse_range(g, field[0], field[1]) != 0) {
                goto error;
            }

        } else if (geo_mode == GEO_CIDR) {
            /* TODO */
        } else {
            /* never get here */
        }
    }

    return g;

error:
    geo_unload(g);
    return NULL;
}

void geo_unload(geo_t *g, int geo_mode) {
    if (geo_mode == GEO_RANGE) {
        range_free(g->u.range);
    } else if (geo_mode == GEO_CIDR) {
        /* TODO */
    } else {
        /* never get here */
    }

    free(g);
}

/* gcc -D GEO_TEST_MAIN geo.c range.c radix.c vector.c */
#ifdef GEO_TEST_MAIN
int main(int argc, char **argv) {
    geo_t *g = geo_load("fnsproxy.geo", GEO_RANGE);
    assert(g);
}
#endif /* GEO_TEST_MAIN */
