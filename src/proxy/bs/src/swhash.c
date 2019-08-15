//#include "swapi.h"
//#include "swmem.h"
#include "swhash.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

//////////////////////////////////////////////////////////////////////////////////////////
// base struct

/* hash表操作结构 */
struct hash_ops
{
    int32_t (*insert)(hash_t, const void *, int32_t, const void *);           // 插入
    int32_t (*delete)(hash_t, const void *, int32_t, int);                   // 删除
    int32_t (*replace)(hash_t, const void *, int32_t, const void *, void **); // 替换
    void   *(*lookup)(hash_t, const void *, int32_t);                         // 查找
    void    (*visit)(hash_t, hash_visit_cb, void *);                                  // 遍历hash表
    void    (*destroy)(hash_t, int);                                         // 销毁
};

/* hash表结构 */
struct shash_t
{
    uint32_t tsize;              // 表长
    uint32_t ksize;              // key值长度，默认0为变长
    uint32_t nelems;             // hash表中已存在元素的数目
    uint32_t nElemCollisionCnts; // hash表中元素发生碰撞的次数(mixed-value)

    struct hash_ops *h_ops;      // hash操作结构
    hash_ex_func_hash hash_fn;   // hash函数
    hash_ex_func_cmp cmp_fn;     // hash比较key函数
    hash_data_free_cb free_cb;   // hash释放data函数

    union
    {
        struct elem **chtable;   // 链表类型
    } dm;                        // hash表数据管理者(无论哪种类型,都必须解决碰撞冲突的问题)
};

//////////////////////////////////////////////////////////////////////////////////////////
// "chain hash" struct

struct elem
{
    void *key;          // 关键字
    int32_t keylen;     // 关键字长度
    void *data;         // 数据指针
    struct elem *next;  // 指向下一个元素
};

static int32_t ch_insert(hash_t, const void *, int32_t, const void *);
static int32_t ch_delete(hash_t, const void *, int32_t, int);
static int32_t ch_replace(hash_t, const void *, int32_t, const void *, void **);
static void *ch_lookup(hash_t, const void *, int32_t);
static void ch_visit(hash_t, hash_visit_cb, void *);
static void ch_destroy(hash_t, int);

struct hash_ops ch_ops = {
    .insert = ch_insert,
    .delete = ch_delete,
    .replace = ch_replace,
    .lookup = ch_lookup,
    .visit = ch_visit,
    .destroy = ch_destroy,
};

//////////////////////////////////////////////////////////////////////////////////////////
// hash functions

/* bob hash(http://burtleburtle.net/bob/index.html) */
#define mix(a, b, c) \
{ \
  a -= b; a -= c; a ^= (c >> 13); \
  b -= c; b -= a; b ^= (a << 8);  \
  c -= a; c -= b; c ^= (b >> 13); \
  a -= b; a -= c; a ^= (c >> 12); \
  b -= c; b -= a; b ^= (a << 16); \
  c -= a; c -= b; c ^= (b >> 5);  \
  a -= b; a -= c; a ^= (c >> 3);  \
  b -= c; b -= a; b ^= (a << 10); \
  c -= a; c -= b; c ^= (b >> 15); \
}

static uint32_t bob_hash(const unsigned char *k, uint32_t length)
{
    uint32_t a, b, c, len;

    /* set up the internal state */
    len = length;
    a = b = c = 0x9e3779b9;  // the golden ratio; an arbitrary data

    /* handle most of the key */
    while (len >= 12)
    {
        a += (k[0] +((uint32_t)k[1] << 8) +((uint32_t)k[2] << 16) +((uint32_t)k[3] << 24));
        b += (k[4] +((uint32_t)k[5] << 8) +((uint32_t)k[6] << 16) +((uint32_t)k[7] << 24));
        c += (k[8] +((uint32_t)k[9] << 8) +((uint32_t)k[10]<< 16)+((uint32_t)k[11] << 24));
        mix(a, b, c);
        k += 12; len -= 12;
    }

    /* handle the last 11 bytes */
    c += length;
    switch(len)
    { // all the case statements fall through
        case 11: c+=((uint32_t)k[10] << 24);
        case 10: c+=((uint32_t)k[9]  << 16);
        case 9 : c+=((uint32_t)k[8]  << 8);
        /* the first byte of c is reserved for the length */
        case 8 : b+=((uint32_t)k[7] << 24);
        case 7 : b+=((uint32_t)k[6] << 16);
        case 6 : b+=((uint32_t)k[5] << 8);
        case 5 : b+=k[4];
        case 4 : a+=((uint32_t)k[3] << 24);
        case 3 : a+=((uint32_t)k[2] << 16);
        case 2 : a+=((uint32_t)k[1] << 8);
        case 1 : a+=k[0];
        /* case 0: nothing left to add */
    }
    mix(a, b, c);

    return c;
}

/* one-at-a-time hash */
static uint32_t oneatatime_hash(const unsigned char *k, uint32_t length)
{
    uint32_t hash = 0;
    uint32_t i;

    for(i = 0; i < length; ++i)
    {
        hash += k[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/* time33 hash */
static uint32_t time33_hash(const unsigned char *k, uint32_t length)
{
    uint32_t hash = 5381;
    uint32_t i = 0;

    for(i = 0; i < length; i++)
    {
        hash = (hash<<5) + hash + k[i]; // hash = hash*31 + k[i];
    }

    return hash;
}

/* mixed computing imaging to my hash table */
static uint32_t getHash(hash_t htable, const void *key, int32_t keylen) 
{
    int32_t len;
    if(htable->ksize == 0) len = keylen;
    else len = htable->ksize;
    return ((htable->hash_fn(key, len)) & (htable->tsize-1)); // hash the key and get the meaningful bits for our table
}

//////////////////////////////////////////////////////////////////////////////////////////
// default hash key compare functions

static int32_t default_hash_key_cmp(const void *p1, int32_t len1,const void *p2, int32_t len2)
{
    if(len1 != len2) return -1;
    else return memcmp(p1, p2, len1); // return strncmp((const char * )p1, (const char * )p2, len1);
}

//////////////////////////////////////////////////////////////////////////////////////////
// main functions

hash_t sw_hash_create(uint32_t sizehint, uint32_t keysize, enum hash_alg alg, hash_ex_func_hash hfunc, hash_ex_func_cmp cfunc, hash_data_free_cb fcb, uint32_t flag)
{
    hash_t htable;
    uint32_t size = 0; // htable size
    uint32_t i = 1;

    /* 1, take the size hint and round it to the next higher power of two */
    while(size < sizehint)
    {
        size = 1<<i++;
        if(size == 0) { size = 1<<(i-2); break; }
    }

    /* 2, create hash htable */
    htable = (hash_t)malloc(sizeof(struct shash_t));
    if(!htable) { errno = ENOMEM; goto ErrorP; }

    /* 3, initialize hash htable common fields */
    htable->tsize = size;
    htable->ksize = keysize;
    htable->nelems = 0;
    htable->nElemCollisionCnts = 0;
    switch(alg)
    {
        case BOB_HASH:
            htable->hash_fn = bob_hash;
            break;
        case ONEATATIME_HASH:
            htable->hash_fn = oneatatime_hash;
            break;
        case TIME33_HASH:
            htable->hash_fn = time33_hash;
            break;
        default:
            if(hfunc) htable->hash_fn = hfunc;
            else {
                errno = EINVAL;
                goto ErrorP;
            }
    }
    if(cfunc) htable->cmp_fn = cfunc;
    else htable->cmp_fn = default_hash_key_cmp;
    htable->free_cb = fcb;

    /* 4, create elements for each hash htable type */
    switch(flag)
    {
        case CHAIN_H:
            htable->h_ops = &ch_ops;
            htable->dm.chtable = (struct elem **)malloc(size*sizeof(struct elem *));
            if(!htable->dm.chtable) { errno = ENOMEM; goto ErrorP; }
            else memset(htable->dm.chtable, '\0', size*sizeof(struct elem *));
            break;
        default:
            errno = EINVAL;
            goto ErrorP;
    }

    return htable;

ErrorP:
    if(htable && flag == CHAIN_H && htable->dm.chtable) free((void *)htable->dm.chtable);
    if(htable) free((void *)htable);
    return NULL;
}

int32_t sw_hash_insert(hash_t htable, const void *key, int32_t keylen, const void *data)
{
    assert(htable);
    assert(key);
    assert(keylen);
    assert(data);
    return htable->h_ops->insert(htable, key, keylen, data);
}

int32_t sw_hash_delete(hash_t htable, const void *key, int32_t keylen, int bAutoFreeData)
{
    assert(htable);
    assert(key);
    assert(keylen);
    return htable->h_ops->delete(htable, key, keylen, bAutoFreeData);
}

int32_t sw_hash_replace(hash_t htable, const void *key, int32_t keylen, const void *newdata, void **olddata)
{
    assert(htable);
    assert(key);
    assert(keylen);
    assert(newdata);
    assert(olddata);
    return htable->h_ops->replace(htable, key, keylen, newdata, olddata);
}

void *sw_hash_lookup(hash_t htable, const void *key, int32_t keylen)
{
    assert(htable);
    assert(key);
    assert(keylen);
    return htable->h_ops->lookup(htable, key, keylen);
}

uint32_t sw_hash_getTableSize(hash_t htable)
{
    assert(htable);
    return htable->tsize;
}

uint32_t sw_hash_getElemsCnt(hash_t htable)
{
    assert(htable);
    return htable->nelems;
}

uint32_t sw_hash_getElemCollisionCnts(hash_t htable)
{
    assert(htable);
    return htable->nElemCollisionCnts;
}

void sw_hash_visit(hash_t htable, hash_visit_cb vcb, void *param)
{
    assert(htable);
    htable->h_ops->visit(htable, vcb, param);
}

void sw_hash_destroy(hash_t htable, int bAutoFreeData)
{
    assert(htable);
    htable->h_ops->destroy(htable, bAutoFreeData);
    free((void *)htable);
}

//////////////////////////////////////////////////////////////////////////////////////////
// "chain hash" functions

static int32_t ch_insert(hash_t htable, const void *key, int32_t keylen, const void *data)
{
    uint32_t idx;
    struct elem *cursor = NULL;
    struct elem *newelem = NULL;
    void *keycopy = NULL;

    newelem = (struct elem *)malloc(sizeof(struct elem));
    if(!newelem) { errno = ENOMEM; goto ErrorP; }

    keycopy = (struct elem *)malloc(keylen*sizeof(char));
    if(!keycopy) { errno = ENOMEM; goto ErrorP; }
    else memcpy(keycopy, key, keylen);

    newelem->key = keycopy;
    newelem->keylen = keylen;
    newelem->data = (void *)data;
    newelem->next = NULL;

    idx = getHash(htable, key, keylen);
    if(htable->dm.chtable[idx] == NULL)
    { // no collision, first element in this bucket
        htable->dm.chtable[idx] = newelem;
    }
    else
    { // collision, insert at the end of the chain
        cursor = htable->dm.chtable[idx];
        while(1)
        {
            if(htable->cmp_fn(cursor->key, cursor->keylen, newelem->key, keylen) == 0) 
            { // key is already inserted in the table, insert fails
                errno = EINVAL;
                goto ErrorP;
            }
            if(cursor->next == NULL) break;
            else cursor = cursor->next;
        } 
        cursor->next = newelem; // insert element at the end of the chain
        htable->nElemCollisionCnts++;
    }

    htable->nelems++;
    return 0;

ErrorP:
    if(newelem && newelem->key) free((void *)newelem->key);
    if(newelem) free((void *)newelem);
    return -1;
}

static int32_t ch_delete(hash_t htable, const void *key, int32_t keylen, int bAutoFreeData)
{
    uint32_t idx;
    struct elem *cursor;
    struct elem *delelem;
    int bElemCollisionDecrement;

    idx = getHash(htable, key, keylen);
    if(!htable->dm.chtable[idx]) return 0;

    delelem = NULL, bElemCollisionDecrement = 0;
    if(htable->cmp_fn(htable->dm.chtable[idx]->key, htable->dm.chtable[idx]->keylen, key, keylen) == 0) 
    { // element to delete is the first in the chain
        delelem = htable->dm.chtable[idx];
        htable->dm.chtable[idx] = delelem->next;
        if(htable->dm.chtable[idx]) bElemCollisionDecrement = 1;
    }
    else
    { // search thru the chain for the element
        cursor = htable->dm.chtable[idx];
        while(cursor->next)
        {
            if(htable->cmp_fn(cursor->next->key, cursor->next->keylen, key, keylen) == 0)
            {
                delelem = cursor->next;
                cursor->next = delelem->next;
                bElemCollisionDecrement = 1;
                break;
            }
            cursor = cursor->next;
        }
    }

    if(delelem)
    {
        free((void *)delelem->key);
        free((void *)delelem);
        if(bAutoFreeData && htable->free_cb) htable->free_cb((void *)delelem->data);

        htable->nelems--;
        if(bElemCollisionDecrement) htable->nElemCollisionCnts--;
    }

    return 0;
}

static int32_t ch_replace(hash_t htable, const void *key, int32_t keylen, const void *newdata, void **olddata)
{
    uint32_t idx;
    struct elem *cursor;

    idx = getHash(htable, key, keylen);
    cursor = htable->dm.chtable[idx];
    while(cursor)
    {
        if(htable->cmp_fn(cursor->key, cursor->keylen, key, keylen) == 0) 
        {
            *olddata = cursor->data;
            cursor->data = (void *)newdata;
            return 0;
        }
        cursor = cursor->next;
    }

    return -1;
}

static void *ch_lookup(hash_t htable, const void *key, int32_t keylen)
{
    uint32_t idx;
    struct elem *cursor;

    idx = getHash(htable, key, keylen);
    cursor = htable->dm.chtable[idx];
    while(cursor)
    {
        if(htable->cmp_fn(cursor->key, cursor->keylen, key, keylen) == 0) return cursor->data;	
        cursor = cursor->next;
    }

    return NULL;
}

static void ch_visit(hash_t htable, hash_visit_cb vcb, void *param)
{
    uint32_t idx;
    struct elem *cursor; 

    for(idx = 0; idx < htable->tsize; idx++) 
    {
        cursor = htable->dm.chtable[idx];
        while(cursor)
        {
            if(vcb) vcb(cursor->data, param);
            cursor = cursor->next;
        }
    }
}

static void ch_destroy(hash_t htable, int bAutoFreeData)
{
    uint32_t idx;
    struct elem *cursor, *delelem;

    for(idx = 0; idx < htable->tsize; idx++) 
    {	
        cursor = htable->dm.chtable[idx];
        while(cursor) 
        {
            delelem = cursor;
            cursor = cursor->next;

            free((void *)delelem->key);
            if(bAutoFreeData && htable->free_cb) htable->free_cb(delelem->data);
            free((void *)delelem);
        }
    }
    free((void *)htable->dm.chtable);
}

