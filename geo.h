#ifndef __GEO_H_INCLUDED__
#define __GEO_H_INCLUDED__

#include "range.h"
#include "radix.h"

#define GEO_RANGE   0    
#define GEO_CIDR    1

typedef struct geo_st {
    union {
        range_t     *range;
        radix_t     *tree;
    } u;
} geo_t;

geo_t *geo_load(char *filename, int geo_mode);
void geo_unload(geo_t *g, int geo_mode);
uint32_t geo_get(geo_t *g, uint32_t ip);

#endif /* __GEO_H_INCLUDED__ */
