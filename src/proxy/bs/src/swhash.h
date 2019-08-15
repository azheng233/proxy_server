/**
 *@file swhash.h
 *@brief 哈希表
 *@author niujiuru
 *data 2014-03-17
 */
#ifndef __SWHASH_H__
#define __SWHASH_H__
#include <stdint.h>

/* hash算法 */
enum hash_alg {
    BOB_HASH,
    ONEATATIME_HASH,
    TIME33_HASH,
    EXTEND_HASH, // 外部提供的hash算法
};

/* hash表类型(实现方式) */
#define CHAIN_H 0U     // chain hash (ch)

/* hash算法外部扩展函数指针 */
typedef uint32_t (*hash_ex_func_hash)(const unsigned char *key, uint32_t keylen);

/* key字比较外部扩展函数指针 */
typedef int32_t (*hash_ex_func_cmp)(const void *key1, int32_t key1len, const void *key2, int32_t key2len);

/* data空间释放回调函数 */
typedef void (*hash_data_free_cb)(void *data);

/* visit遍历回调函数 */
typedef void (*hash_visit_cb)(void *data, void *param);

/* hash表 */
typedef struct shash_t *hash_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 创建一个HASH表
 *
 * @param[in] size     hash表长度
 * @param[in] keysize  关键字长度, 默认0为变长
 * @param[in] alg      hash算法
 * @param[in] hfunc    扩展的hash算法函数, "alg"为"EXTEND_HASH"时有效
 * @param[in] cfunc    扩展的关键字比较函数, 默认NULL为逐字符比较
 * @param[in] fcb      data空间释放回调函数
 * @param[in] flag     hash碰撞冲突处理方式, 默认0为"CHAIN_H"
 *
 * @return hash_t
 * @retval NULL 申请失败
 */
hash_t sw_hash_create(uint32_t size, uint32_t keysize, enum hash_alg alg, hash_ex_func_hash hfunc, hash_ex_func_cmp cfunc, hash_data_free_cb fcb, uint32_t flag);

/**
 * @brief 添加一条记录
 *
 * @param[in] htable 哈希表
 * @param[in] key    关键字
 * @param[in] keylen 关键字长度
 * @param[in] data   数据
 *
 * @return 0   成功
 * @retval -1 添加失败
 */
int32_t sw_hash_insert(hash_t htable, const void *key, int32_t keylen, const void *data);

/**
 * @brief 删除一条记录
 *
 * @param[in]  htable  哈希表
 * @param[in]  key     关键字
 * @param[in]  keylen  关键字长度
 * @param[in]  bAutoFreeData 是否自动调用创建hash表时设定的"hash_data_free_cb"回调函数释放数据内存
 *
 * @return 0  成功
 * @retval -1 删除失败
 */
int32_t sw_hash_delete(hash_t htable, const void *key, int32_t keylen, int bAutoFreeData);

/**
 * @brief 替换key的data值
 *
 * @param[in]  htable    哈希表
 * @param[in]  key       关键字
 * @param[in]  keylen    关键字长度
 * @param[in]  newdata   替换值
 * @param[out] olddata   被替换字指针的指针(data associated to this data MUST be freed by the caller)
 *
 * @return 0   成功
 * @retval -1  替换失败
 */
int32_t sw_hash_replace(hash_t htable, const void *key, int32_t keylen, const void *newdata, void **olddata);

/**
 * @brief 查找一条记录
 *
 * @param[in] htable 哈希表
 * @param[in] key    关键字
 * @param[in] keylen 关键字长度
 *
 * @return data  成功,返回查找到的记录
 * @retval NULL  查找失败
 */
void *sw_hash_lookup(hash_t htable, const void *key, int32_t keylen);

/** @brief 取得一个hash表长度 */
uint32_t sw_hash_getTableSize(hash_t htable);

/** @brief 取得一个hash表已存在元素的数目 */
uint32_t sw_hash_getElemsCnt(hash_t htable);

/** @brief 取得一个hash表中元素发生碰撞的次数(mixed-value) */
uint32_t sw_hash_getElemCollisionCnts(hash_t htable);

/** @brief 遍历一个hash表 */
void sw_hash_visit(hash_t htable, hash_visit_cb vcb, void *param);

/** @breif 销毁一个hash表("bAutoFreeData"-是否自动调用创建hash表时设定的"hash_data_free_cb"回调函数释放数据内存) */
void sw_hash_destroy(hash_t htable, int bAutoFreeData);

#ifdef __cplusplus
}
#endif

#endif  /* __SWHASH_H__ */
