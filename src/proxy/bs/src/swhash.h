/**
 *@file swhash.h
 *@brief ��ϣ��
 *@author niujiuru
 *data 2014-03-17
 */
#ifndef __SWHASH_H__
#define __SWHASH_H__
#include <stdint.h>

/* hash�㷨 */
enum hash_alg {
    BOB_HASH,
    ONEATATIME_HASH,
    TIME33_HASH,
    EXTEND_HASH, // �ⲿ�ṩ��hash�㷨
};

/* hash������(ʵ�ַ�ʽ) */
#define CHAIN_H 0U     // chain hash (ch)

/* hash�㷨�ⲿ��չ����ָ�� */
typedef uint32_t (*hash_ex_func_hash)(const unsigned char *key, uint32_t keylen);

/* key�ֱȽ��ⲿ��չ����ָ�� */
typedef int32_t (*hash_ex_func_cmp)(const void *key1, int32_t key1len, const void *key2, int32_t key2len);

/* data�ռ��ͷŻص����� */
typedef void (*hash_data_free_cb)(void *data);

/* visit�����ص����� */
typedef void (*hash_visit_cb)(void *data, void *param);

/* hash�� */
typedef struct shash_t *hash_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief ����һ��HASH��
 *
 * @param[in] size     hash����
 * @param[in] keysize  �ؼ��ֳ���, Ĭ��0Ϊ�䳤
 * @param[in] alg      hash�㷨
 * @param[in] hfunc    ��չ��hash�㷨����, "alg"Ϊ"EXTEND_HASH"ʱ��Ч
 * @param[in] cfunc    ��չ�Ĺؼ��ֱȽϺ���, Ĭ��NULLΪ���ַ��Ƚ�
 * @param[in] fcb      data�ռ��ͷŻص�����
 * @param[in] flag     hash��ײ��ͻ����ʽ, Ĭ��0Ϊ"CHAIN_H"
 *
 * @return hash_t
 * @retval NULL ����ʧ��
 */
hash_t sw_hash_create(uint32_t size, uint32_t keysize, enum hash_alg alg, hash_ex_func_hash hfunc, hash_ex_func_cmp cfunc, hash_data_free_cb fcb, uint32_t flag);

/**
 * @brief ���һ����¼
 *
 * @param[in] htable ��ϣ��
 * @param[in] key    �ؼ���
 * @param[in] keylen �ؼ��ֳ���
 * @param[in] data   ����
 *
 * @return 0   �ɹ�
 * @retval -1 ���ʧ��
 */
int32_t sw_hash_insert(hash_t htable, const void *key, int32_t keylen, const void *data);

/**
 * @brief ɾ��һ����¼
 *
 * @param[in]  htable  ��ϣ��
 * @param[in]  key     �ؼ���
 * @param[in]  keylen  �ؼ��ֳ���
 * @param[in]  bAutoFreeData �Ƿ��Զ����ô���hash��ʱ�趨��"hash_data_free_cb"�ص������ͷ������ڴ�
 *
 * @return 0  �ɹ�
 * @retval -1 ɾ��ʧ��
 */
int32_t sw_hash_delete(hash_t htable, const void *key, int32_t keylen, int bAutoFreeData);

/**
 * @brief �滻key��dataֵ
 *
 * @param[in]  htable    ��ϣ��
 * @param[in]  key       �ؼ���
 * @param[in]  keylen    �ؼ��ֳ���
 * @param[in]  newdata   �滻ֵ
 * @param[out] olddata   ���滻��ָ���ָ��(data associated to this data MUST be freed by the caller)
 *
 * @return 0   �ɹ�
 * @retval -1  �滻ʧ��
 */
int32_t sw_hash_replace(hash_t htable, const void *key, int32_t keylen, const void *newdata, void **olddata);

/**
 * @brief ����һ����¼
 *
 * @param[in] htable ��ϣ��
 * @param[in] key    �ؼ���
 * @param[in] keylen �ؼ��ֳ���
 *
 * @return data  �ɹ�,���ز��ҵ��ļ�¼
 * @retval NULL  ����ʧ��
 */
void *sw_hash_lookup(hash_t htable, const void *key, int32_t keylen);

/** @brief ȡ��һ��hash���� */
uint32_t sw_hash_getTableSize(hash_t htable);

/** @brief ȡ��һ��hash���Ѵ���Ԫ�ص���Ŀ */
uint32_t sw_hash_getElemsCnt(hash_t htable);

/** @brief ȡ��һ��hash����Ԫ�ط�����ײ�Ĵ���(mixed-value) */
uint32_t sw_hash_getElemCollisionCnts(hash_t htable);

/** @brief ����һ��hash�� */
void sw_hash_visit(hash_t htable, hash_visit_cb vcb, void *param);

/** @breif ����һ��hash��("bAutoFreeData"-�Ƿ��Զ����ô���hash��ʱ�趨��"hash_data_free_cb"�ص������ͷ������ڴ�) */
void sw_hash_destroy(hash_t htable, int bAutoFreeData);

#ifdef __cplusplus
}
#endif

#endif  /* __SWHASH_H__ */
