#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>     //for offsetof()
#include "iprbtree.h"
#include "rbtree.h"
#include "log.h"

typedef struct _IpBiTreeHead_s
{
    int 		total_ipitem;
    rbtree_t	IpBiTree;

}IpBiTreeHead_t;

typedef struct _IpBiTreeNode_s
{
    rbtree_node_t	ip_rbtree_node;

}IpBiTreeNode_t;

static IpBiTreeHead_t IpBiTreeHeader;
static rbtree_node_t	s;

int init_rbtree()
{
    IpBiTreeHeader.total_ipitem = 0;
    rbtree_init(&(IpBiTreeHeader.IpBiTree), &(s));

    return 0;
}

int insert_policy_to_rbtree(uint32_t ipport, struct policy_node *pnode)
{
    //Not been tested locally, and if the node to be inserted already exists, delete existing nodes
    uint32_t  tmp_key = ipport;
    rbtree_node_t   *p_is_existed_node = NULL;
    IpBiTreeNode_t *p_tmp_tree_node;

    p_tmp_tree_node = (IpBiTreeNode_t*)malloc(sizeof(IpBiTreeNode_t));
    if (p_tmp_tree_node == NULL) {
        return -1;
    }

    p_is_existed_node = rbtree_find(&(IpBiTreeHeader.IpBiTree), tmp_key);
    if (p_is_existed_node) {
        log_debug("insert ipport policy node to tree failed, alread exsit, ipport: %u", ipport);
        delete_policy_from_rbtree(ipport);
    }
    p_tmp_tree_node->ip_rbtree_node.key = tmp_key;
    p_tmp_tree_node->ip_rbtree_node.left = NULL;
    p_tmp_tree_node->ip_rbtree_node.right = NULL;
    p_tmp_tree_node->ip_rbtree_node.parent = NULL;
    p_tmp_tree_node->ip_rbtree_node.data = (void *)pnode;
    p_tmp_tree_node->ip_rbtree_node.color = 0;

    //Insert IP node to the IP RBTREE,Increase the number of nodes
    rbtree_insert(&(IpBiTreeHeader.IpBiTree), &(p_tmp_tree_node->ip_rbtree_node));
    IpBiTreeHeader.total_ipitem++;

    log_debug("insert ipport policy node to tree, ipport: %u", ipport);
    return 0;
}

int delete_policy_from_rbtree(uint32_t ipport)
{
    uint32_t  tmp_key = ipport;
    IpBiTreeNode_t *p_tmp_tree_node = NULL;
    rbtree_node_t	*p_tmp_node = NULL;

    p_tmp_node = rbtree_find(&(IpBiTreeHeader.IpBiTree), tmp_key);
    if (!p_tmp_node) {
        log_error("can't find node in tree by ipport: %u", ipport);
        return -1;
    }

    //According to the return address,then according to the offset to find the structure of the head
    p_tmp_tree_node = (IpBiTreeNode_t *)((unsigned char *)p_tmp_node - offsetof(IpBiTreeNode_t, ip_rbtree_node));
    rbtree_delete(&(IpBiTreeHeader.IpBiTree), p_tmp_node);

    if (p_tmp_tree_node) {
        free(p_tmp_tree_node);
        p_tmp_tree_node = NULL;
        log_debug("delete node in tree by ipport: %u", ipport);
    } else {
        log_debug("delete node null in tree by ipport: %u", ipport);
        return -1;
    }

    IpBiTreeHeader.total_ipitem--;
    return 0;
}

struct policy_node *find_policy_in_rbtree(uint32_t ipport)
{
    uint32_t  tmp_key = ipport;
    IpBiTreeNode_t *p_tmp_tree_node = NULL;
    rbtree_node_t   *p_tmp_node = NULL;

    p_tmp_node = rbtree_find(&(IpBiTreeHeader.IpBiTree), tmp_key);
    if (!p_tmp_node) {
        log_debug("can't find node in tree by ipport: %u", ipport);
        return NULL;
    }

    log_debug("find node in tree by ipport: %u", ipport);
    //According to the return address,then according to the offset to find the structure of the head
    p_tmp_tree_node = (IpBiTreeNode_t *)((unsigned char *)p_tmp_node - offsetof(IpBiTreeNode_t, ip_rbtree_node));
    return (struct policy_node *)p_tmp_tree_node->ip_rbtree_node.data;
}

void mid_visit_rbtree()
{
    mid_visit(IpBiTreeHeader.IpBiTree.root, IpBiTreeHeader.IpBiTree.sentinel);
}

void mid_destroy_rbtree(rbtree_ex_func free_data)
{
    mid_deletetree(IpBiTreeHeader.IpBiTree.root, IpBiTreeHeader.IpBiTree.sentinel, free_data);
    IpBiTreeHeader.IpBiTree.root = NULL;
    IpBiTreeHeader.IpBiTree.sentinel = NULL;
}
