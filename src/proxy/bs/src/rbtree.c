#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rbtree.h"
#include "log.h"

void rbtree_left_rotate(rbtree_node_t **root, rbtree_node_t *sentinel,
					   rbtree_node_t *node)
{
    rbtree_node_t  *temp;
	
    temp = node->right;
    node->right = temp->left;
	
    if (temp->left != sentinel) {
        temp->left->parent = node;
    }
	
    temp->parent = node->parent;
	
    if (node == *root) {
        *root = temp;
		
    } else if (node == node->parent->left) {
        node->parent->left = temp;
		
    } else {
        node->parent->right = temp;
    }
	
    temp->left = node;
    node->parent = temp;
}


void rbtree_right_rotate(rbtree_node_t **root, rbtree_node_t *sentinel,
						rbtree_node_t *node)
{
    rbtree_node_t  *temp;
	
    temp = node->left;
    node->left = temp->right;
	
    if (temp->right != sentinel) {
        temp->right->parent = node;
    }
	
    temp->parent = node->parent;
	
    if (node == *root) {
        *root = temp;
		
    } else if (node == node->parent->right) {
        node->parent->right = temp;
		
    } else {
        node->parent->left = temp;
    }
	
    temp->right = node;
    node->parent = temp;
}


void rbtree_insert_temp(rbtree_node_t *temp, rbtree_node_t *node,
						rbtree_node_t *sentinel)
{
    rbtree_node_t  **p;
	
    for ( ;; ) {
		
        p = (node->key < temp->key) ? &temp->left : &temp->right;
		
        if (*p == sentinel) {
            break;
        }
		
        temp = *p;
    }
	
    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    rbt_red(node);
}



void rbtree_insert(rbtree_t *tree,rbtree_node_t *node)
{
    rbtree_node_t  **root, *temp, *sentinel;
	
    /* a binary tree insert */
	
    root = (rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;
	
    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        rbt_black(node);
        *root = node;
		
        return;
    }
	
    //tree->insert(*root, node, sentinel);
	rbtree_insert_temp(*root, node, sentinel);
    /* re-balance tree */
	
    while (node != *root && rbt_is_red(node->parent)) {
		
        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;
			
            if (rbt_is_red(temp)) {
                rbt_black(node->parent);
                rbt_black(temp);
                rbt_red(node->parent->parent);
                node = node->parent->parent;
				
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rbtree_left_rotate(root, sentinel, node);
                }
				
                rbt_black(node->parent);
                rbt_red(node->parent->parent);
                rbtree_right_rotate(root, sentinel, node->parent->parent);
            }
			
        } else {
            temp = node->parent->parent->left;
			
            if (rbt_is_red(temp)) {
                rbt_black(node->parent);
                rbt_black(temp);
                rbt_red(node->parent->parent);
                node = node->parent->parent;
				
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rbtree_right_rotate(root, sentinel, node);
                }
				
                rbt_black(node->parent);
                rbt_red(node->parent->parent);
                rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }
	
    rbt_black(*root);
}

rbtree_node_t *rbtree_min(rbtree_node_t *node, rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }
	
    return node;
}

void rbtree_delete(rbtree_t *tree,rbtree_node_t *node)
{
    unsigned char           red;
    rbtree_node_t  **root, *sentinel, *subst, *temp, *w;

    /* a binary tree delete */

    root = (rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;

    if (node->left == sentinel) {
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) {
        temp = node->left;
        subst = node;

    } else {
        subst = rbtree_min(node->right, sentinel);

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) {
        *root = temp;
        rbt_black(temp);

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }

    red = rbt_is_red(subst);

    if (subst == subst->parent->left) {
        subst->parent->left = temp;

    } else {
        subst->parent->right = temp;
    }

    if (subst == node) {

        temp->parent = subst->parent;

    } else {

        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        rbt_copy_color(subst, node);

        if (node == *root) {
            *root = subst;

        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;

    if (red) {
        return;
    }

    /* a delete fixup */

    while (temp != *root && rbt_is_black(temp)) {

        if (temp == temp->parent->left) {
            w = temp->parent->right;

            if (rbt_is_red(w)) {
                rbt_black(w);
                rbt_red(temp->parent);
                rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (rbt_is_black(w->left) && rbt_is_black(w->right)) {
                rbt_red(w);
                temp = temp->parent;

            } else {
                if (rbt_is_black(w->right)) {
                    rbt_black(w->left);
                    rbt_red(w);
                    rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                rbt_copy_color(w, temp->parent);
                rbt_black(temp->parent);
                rbt_black(w->right);
                rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else {
            w = temp->parent->left;

            if (rbt_is_red(w)) {
                rbt_black(w);
                rbt_red(temp->parent);
                rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (rbt_is_black(w->left) && rbt_is_black(w->right)) {
                rbt_red(w);
                temp = temp->parent;

            } else {
                if (rbt_is_black(w->left)) {
                    rbt_black(w->right);
                    rbt_red(w);
                    rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                rbt_copy_color(w, temp->parent);
                rbt_black(temp->parent);
                rbt_black(w->left);
                rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    rbt_black(temp);
}

rbtree_node_t *rbtree_find( rbtree_t *t, keytype key )
{
	struct rbtree_node_s *tmpnode;
	struct rbtree_node_s *sentinel;
	//int i;
	tmpnode = t->root;
    sentinel = t->sentinel;
	//i = 0;
    while (tmpnode != sentinel) {
		
        if (key < tmpnode->key) {
            tmpnode = tmpnode->left;
			//log_debug("%d\n",i++);
            continue;
        }
		
        if (key > tmpnode->key) {
            tmpnode = tmpnode->right;
			//log_debug("%d\n",i++);
            continue;
        }
		//log_debug("find");
		return tmpnode;
	}
	return NULL;
}

void mid_visit( rbtree_node_t *root, rbtree_node_t *sentinel )
{
	if( sentinel != root )
	{
		if( sentinel != root->left )
		{
			mid_visit( root->left, sentinel );
			log_debug("left: %d\n", root->left->key );
		}
		if( NULL != root->parent )
			log_debug("parent: %d\n", root->parent->key );
		log_debug("key: %d\tvalue: %s\n", root->key, root->data );
		if( sentinel != root->right )
		{
			log_debug("right: %d\n", root->right->key );
			mid_visit( root->right, sentinel );
		}
	}
}

void mid_deletetree( rbtree_node_t *root, rbtree_node_t *sentinel, rbtree_ex_func free_data )
{
	rbtree_node_t *ptmp = NULL;
	if( NULL != root )
	{
		if( sentinel != root->left )
			mid_deletetree( root->left, sentinel, free_data );
		ptmp = root->right;
		/*if( (NULL!=free_data)&&(NULL!=root->data) )
			free_data( root->data );*/
		root->data = NULL;
		if( (NULL != root)&&(sentinel != root) )
		{
			free( root );
			root = NULL;
		}
		if( sentinel != ptmp )
			mid_deletetree( ptmp, sentinel, free_data );
	}
}
