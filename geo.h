#ifndef __GEO_H_INCLUDED__
#define __GEO_H_INCLUDED__

#include "range.h"
#include "radix.h"

#define GEO_RANGE   0    
#define GEO_CIDR    1

typedef struct geo_st {
    union {
        range_t         *range;
        radix_tree_t    *tree;
    } u;
    int                 geo_mode;
} geo_t;

geo_t *geo_load(geo_t *g, char *filename, int geo_mode);
void geo_unload(geo_t *g);
uint32_t geo_get(geo_t *g, uint32_t ip);
char *geo_get_str(geo_t *g, char *ip_str);

#endif /* __GEO_H_INCLUDED__ */
