#ifndef __RANGE_H_INCLUDED__
#define __RANGE_H_INCLUDED__

#include <stdint.h>

typedef struct {
    void        *value;
    uint16_t    start;
    uint16_t    end;
} range_low_t;

typedef struct {
    range_low_t **low;
    void        *default_value;
} range_t;

range_t *range_load(char *filename);
void range_free(range_t *r);
void *range_find(range_t *r, uint32_t key);


#endif /* __RANGE_H_INCLUDED__ */
