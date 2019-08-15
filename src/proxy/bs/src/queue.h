/** 
 * @file queue.h
 * @brief 队列操作定义
 *
 * Copyright (C) 2009-2010 信大捷安信息技术有限公司版本所有
 *
 * @author hps
 * @version 1.0
 * @date 2010-06-03
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_
#include <stddef.h>

/**
 * @defgroup queue
 * @brief 通用队列操作
 *
 * 封装双向循环队列及其操作
 */
///@{

/** 队列指针结构体 */
typedef struct queue_s queue_t;
struct queue_s{
    struct queue_s  *prev;
    struct queue_s  *next;
} ;

/** 
 * @brief 队列初始化 
 * @param[in] q 队列节点指针 
 */
#define queue_init(q)	\
    (q)->prev = (q);	\
    (q)->next = (q)

/**判断队列是否为空 */
#define queue_isempty(q) \
    ((q)->next==(q) || (q)->prev==(q))

/** 清空队列 */
#define queue_empty(h)	\
    ((h) = (h)->prev)

/** 在元素后添加 */
#define queue_insert_head(h, x)		\
    (x)->next = (h)->next;	\
    (x)->next->prev = (x);	\
    (x)->prev = (h);	\
    (h)->next = (x)

/** 向队列尾添加 */
#define queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = (x);                                                      \
    (x)->next = (h);                                                            \
    (h)->prev = (x)

#define queue_insert_after   queue_insert_head
#define queue_insert_before  queue_insert_tail

/** 返回第一个元素 */
#define queue_head(h)                                                     \
    (h)->next

/** 返回最后一个元素 */
#define queue_last(h)                                                     \
    (h)->prev


#define queue_sentinel(h)                                                 \
    (h)

/** 取下一个元素 */
#define queue_next(q)                                                     \
    (q)->next

/** 取上一个元素 */
#define queue_prev(q)                                                     \
    (q)->prev

/** 删除一个元素 */
#define queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;        \
    queue_init((x))

/** 拆分队列 */
#define queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = (n);                                                      \
    (n)->next = (q);                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = (h);                                                      \
    (q)->prev = (n);

/** 添加一个节点 */
#define queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = (h);

/** 取节点对应数据 */
#define queue_data(q, type, link)                                         \
    (type *) ((unsigned char *)(q) - offsetof(type, link))

///@}
#endif
