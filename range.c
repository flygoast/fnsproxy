#include <stdio.h>
#include <stdint.h>

#define BUFFER_SIZE     2048

typedef struct {
    void        *value;
    uint16_t    start;
    uint16_t    end;
} range_low_t;

typedef struct {
    range_low_t **low;
    void        *default_value;
} range_t;

range_t *range_load(char *filename) {
    FILE *fp = NULL; 
    range_t *r;
    char buf[BUFFER_SIZE];
    char *p;

    if (!filename) {
        return NULL;
    }

    r = (range_t *)malloc(sizeof(*r));
    if (!r) {
        return NULL;
    }

    fp = fopen(filename, "r");
    if (!fp) {
        return NULL;
    }

    while ((p = fgets(buf, sizeof(buf) - 1, fp))) {
        buf[sizeof(buf) - 1] = '\0';
    }
}
void range_free(range_t *r);
void *range_find(range_t *r, uint32_t key);

void range_insert(range_t *r, uint32_t start, uint32_t end, void *value) {
    uint32_t    n;
    uint32_t    h, i, s, e;
    range_low_t *low;

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
    }
}


