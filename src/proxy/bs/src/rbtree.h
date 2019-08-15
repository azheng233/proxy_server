#ifndef _RBTREE_H_
#define _RBTREE_H_

#include <stdint.h>

typedef uint32_t keytype;
typedef  struct rbtree_node_s rbtree_node_t;
typedef void(*rbtree_ex_func)(void *);

struct rbtree_node_s {
    keytype           key;
    rbtree_node_t     *left;
    rbtree_node_t     *right;
    rbtree_node_t     *parent;
    unsigned char     color;
    void			  *data;
} *prbtree_node_t;

typedef struct rbtree_s {
    rbtree_node_t     *root;
    rbtree_node_t     *sentinel;
} rbtree_t, *prbtree_t;

#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)

#define rbtree_init(tree, s)                                           \
    rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s; 

#define rbtree_sentinel_init(node)  rbt_black(node)

void rbtree_insert(rbtree_t *tree, rbtree_node_t *node);
void rbtree_delete(rbtree_t *tree, rbtree_node_t *node);
//NULL means not find.
rbtree_node_t *rbtree_find(rbtree_t *t, keytype key);
void mid_visit(rbtree_node_t *root, rbtree_node_t *sentinel);
void mid_deletetree(rbtree_node_t *root, rbtree_node_t *sentinel, rbtree_ex_func free_data);

#endif
