#ifndef __RADIX_H_INCLUDED__
#define __RADIX_H_INCLUDED__

#include <stdint.h>
#include <inttypes.h>

#define RADIX_NO_VALUE      (unsigned char)-1

typedef struct radix_node_st    radix_node_t;

struct radix_node_st {
    radix_node_t    *right;
    radix_node_t    *left;
    radix_node_t    *parent;
    uint32_t        value;
};

typedef struct {
    radix_node_t    *root;
    size_t          size;
} radix_tree_t;

radix_tree_t *radix_tree_create();

int radix32tree_insert(radix_tree_t *tree, 
        uint32_t key, uint32_t mask, uint32_t value);

int radix32tree_delete(radix_tree_t *tree,
        uint32_t key, uint32_t mask);

uint32_t radix32tree_find(radix_tree_t *tree, uint32_t key);
void radix_tree_free(radix_tree_t *tree);

#endif /* __RADIX_H_INCLUDED__ */
