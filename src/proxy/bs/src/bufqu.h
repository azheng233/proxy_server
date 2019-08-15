#ifndef _MEM_BUFQU_H_
#define _MEM_BUFQU_H_
/** 
 * @file
 * @brief �����������
 * @note ʹ�÷����μ������ļ�@ref test.c
 * @author sunxp
 * @version 0001
 * @date 2010-03-23
 */
/**
 * @mainpage
 * - ����ͽӿ� @ref buffer.h
 * - ʹ��ʾ�� @ref test.c
 *
 * @example test.c
 * ����������ʹ��ʾ��
 */
#include "queue.h"
#include <stddef.h>

/// �������ṹ
typedef struct buffer_qu_st {
    unsigned char *buf; ///< ʵ�ʻ�����
    size_t sum_len;     ///< �ܳ���
	size_t used;		///< ����
    size_t r;           ///< ��ƫ��
    size_t w;           ///< дƫ��
    queue_t queue;      ///< ����ڵ㣬�������Ӵ�������������
} buffer_qu_t;

/// ����������ͷ
typedef struct buffer_queue_st {
    unsigned int sum;   ///< �����л�������Ŀ
    queue_t queue;
} buffer_queue_t;


/** 
 * @brief ����һ���������
 * 
 * @param[out] head �������ͷ
 * 
 * @return �������ṹָ��
 * @retval NULL ʧ��
 */
buffer_queue_t* buffer_queue_create( buffer_queue_t **head );

/** 
 * @brief ���ٻ������
 * 
 * @param[in] head �������ͷ
 * 
 * @retval -1 ʧ��
 * @retval 0 �ɹ�
 */
int buffer_queue_destroy( buffer_queue_t *head );

/** 
 * @brief ��ջ������
 * 
 * @param[in] head �������ͷ
 * 
 * @retval -1 ʧ��
 * @retval 0 �ɹ�
 */
int buffer_queue_empty( buffer_queue_t *head );

/** 
 * @brief �õ������л���������
 * 
 * @param[in] head ����ͷ
 * 
 * @return ����������
 */
 unsigned int buffer_queue_num( buffer_queue_t *head );

/** 
 * @brief ������ڵ�����ͷ
 * 
 * @param[in] head ����ͷ
 * @param[in] buf  ����ڵ�
 */
void buffer_qu_put_head( buffer_queue_t *head, buffer_qu_t *buf );

/** 
 * @brief ������ڵ�����β
 * 
 * @param[in] head ����ͷ
 * @param[in] buf  ����ڵ�
 */
void buffer_put_tail( buffer_queue_t *head, buffer_qu_t *buf );

/** 
 * @brief �Ӷ���ͷ��ȡ��һ���ڵ�
 * 
 * @param[in] head ����ͷ
 * @param[out] buf  ����ڵ�
 * 
 * @return �������ṹָ��
 * @retval NULL ʧ�ܻ����Ϊ��
 */
buffer_qu_t* buffer_get_head( buffer_queue_t *head, buffer_qu_t **buf );

/** 
 * @brief �Ӷ���β��ȡ��һ���ڵ�
 * 
 * @param[in] head ����ͷ
 * @param[out] buf  ����ڵ�
 * 
 * @return �������ṹָ��
 * @retval NULL ʧ�ܻ����Ϊ��
 */
buffer_qu_t* buffer_get_tail( buffer_queue_t *head, buffer_qu_t **buf );


buffer_qu_t *buffer_queue_fetch_first( buffer_queue_t *head );
buffer_qu_t *buffer_queue_fetch_next( buffer_queue_t *head, buffer_qu_t *buf );
buffer_qu_t *buffer_queue_fetch_prev( buffer_queue_t *head, buffer_qu_t *buf );
buffer_qu_t *buffer_queue_fetch_last( buffer_queue_t *head );

buffer_qu_t *buffer_queue_remove( buffer_queue_t *head, buffer_qu_t *buf );


/** 
 * @brief ��ȡָ�����ȵĻ�����
 * 
 * @param[in] len ����������
 * 
 * @return �������ṹָ��
 * @retval NULL ʧ��
 */
buffer_qu_t* buffer_qu_new( size_t len );


/** 
 * @brief �ͷŻ��������ڴ��
 * 
 * @param[in] buf ������
 * 
 * @retval -1 ʧ��
 * @retval 0 �ɹ�
 */
int buffer_qu_del( buffer_qu_t *buf );

/**
 * @brief ��չ��������С
 *
 * @param buf
 *
 * @return 
 */
int buffer_qu_realloc( buffer_qu_t *buf, size_t len );

/** 
 * @brief �õ�������дָ��
 * 
 * @param buf ������
 * 
 * @return дָ��
 */
inline unsigned char* buf_wptr( buffer_qu_t *buf );
/** 
 * @brief �õ���������д����
 * 
 * @param buf ������
 * 
 * @return ��д����
 */
inline size_t buf_wlen( buffer_qu_t *buf );
/** 
 * @brief �õ���������ָ��
 * 
 * @param buf ������
 * 
 * @return ��ָ��
 */
inline unsigned char* buf_rptr( buffer_qu_t *buf );
/** 
 * @brief �õ��������ɶ�����
 * 
 * @param buf ������
 * 
 * @return �ɶ�����
 */
inline size_t buf_rlen( buffer_qu_t *buf );

buffer_qu_t *create_buffer_with_value( unsigned char *value, size_t len );

#endif
