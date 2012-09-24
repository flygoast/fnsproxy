#ifndef __RANGE_H_INCLUDED__
#define __RANGE_H_INCLUDED__

#include <stdint.h>
#include "vector.h"

#define INVALID_IP_ADDR     0

typedef struct {
    uint32_t    ip_addr;
    uint16_t    start;
    uint16_t    end;
} range_low_t;

typedef struct {
    vector_t    **low;
    uint32_t    def_ip;
} range_t;

#define range_set_def(r, d)     (r)->def_ip = (d)

range_t *range_create();
int range_init(range_t *r);
int range_insert(range_t *r, uint32_t start, uint32_t end, uint32_t ip_addr);
int range_insert_str(range_t *r, char *start, char *end, char *ip_addr);
void range_delete(range_t *r, uint32_t start, uint32_t end);
void range_delete_str(range_t *r, char *start, char *end);
uint32_t range_get(range_t *r, uint32_t key);
char *range_get_str(range_t *r, char *key);
void range_destroy(range_t *r);
void range_free(range_t *r);

#endif /* __RANGE_H_INCLUDED__ */
