#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

typedef void (*hash_ex_func)(void *);
typedef void (*hash_ex_func_ex)(void *, void *);
/** 
 * @brief 创建HASH表
 *
 * @param[in] table_len  HASH表初始长度，递增长度
 * 
 * @return 0
 */
int hash_create(unsigned int table_len);

/** 
 * @brief 添加一条记录
 *
 * @param[in] key 关键字 
 * @param[in] key_len 长度
 * @param[in] value 值
 *
 * @return 0
 */
int hash_add(unsigned char *key, int key_len, void *value);

/** 
 * @brief 查找一条记录
 *
 * @param[in] key  
 * @param[in] key_len 长度
 * 
 * @return value
 */
void *hash_search(unsigned char *key, int key_len);

/** 
 * @brief 删除一条记录
 *
 * @param[in] key 关键字 
 * @param[in] key_len 长度
 * 
 * @return 0
 */
int hash_del(unsigned char *key, int key_len);

///销毁HASH表
///
void hash_destroy( hash_ex_func free_data );

//设置Hash表中数据的一个标志，0成功，-1则Hash表为空
int hash_visit( hash_ex_func_ex set_flags, void *pflags );

#endif
