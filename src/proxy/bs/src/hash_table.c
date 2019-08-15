#include <stdio.h>
#include <stdlib.h>
#include "hash_table.h"

#define  HASH_OFFSET  0
#define  HASH_A       1
#define  HASH_B       2

///HASH table node
typedef struct hashtable {
	unsigned int hash_val_a;
	unsigned int hash_val_b;
	int exists;
	int key_len;
	void *value;
} hashtable_t;

///HASH table
static hashtable_t *hashtable = NULL; 
///Hash Seeds
static unsigned int refer_table[0x500] = { 0 };
///the current length of the HASH table
static unsigned int hashtable_cur_len = 0;
///the increased length of the HASH table
static unsigned int hashtable_inc_len = 0;

void init_refer_table(void)
{
	unsigned int seed = 1, index1 = 0, index2 = 0, i;
	unsigned int temp1, temp2;

	for( index1 = 0; index1 < 0x100; index1++ )
	{
		for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
		{
			seed = (seed * 125 + 3) % 0xAAAB;
			temp1 = (seed & 0xFF) << 0x04;
			seed = (seed * 125 + 3) % 0xAAAB;
			temp2 = (seed & 0xFF);
			refer_table[index2] = ( temp1 + temp2 );
		}
	}
}

int hash_alloc()
{
	unsigned int index = 0;
	hashtable_t * new_hashtable = NULL;

	new_hashtable = (hashtable_t *)realloc(hashtable, sizeof(hashtable_t) * (hashtable_cur_len + hashtable_inc_len));
	if (new_hashtable == NULL)
		return -1;

	hashtable = new_hashtable;
	for (index=hashtable_cur_len; index<hashtable_cur_len+hashtable_inc_len; index++)
	{
		hashtable[index].hash_val_a = -1;
		hashtable[index].hash_val_b = -1;
		hashtable[index].exists = 0;
		hashtable[index].key_len =0;
		hashtable[index].value = NULL;
	}
	hashtable_cur_len += hashtable_inc_len;

	return 0;
}

int hash_create( unsigned int table_len )
{
	int ret = 0;

	hashtable = NULL;
	hashtable_cur_len = 0;
	hashtable_inc_len = table_len;

	ret = hash_alloc();
	if(ret != 0)
		return -1;
	
	init_refer_table();
	return 0;
}

unsigned int hash_charset(unsigned char *charset, int charset_len, int hash_type)
{
	unsigned char *key = charset;
	unsigned int seed1 = 0x7FED, seed2 = 0xEEEE;
	unsigned int ch;

	while(charset_len)
	{
		ch = (*key++);
		seed1 = refer_table[(hash_type << 4) + ch] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 2) + 3;
		charset_len--;
	}
	return seed1;
}

int get_add_hash_pos(unsigned int hash_start, unsigned int hash_val_a,unsigned int hash_val_b)
{
	unsigned int hash_pos = hash_start;

	while (hashtable[hash_pos].exists)
	{
		if (hashtable[hash_pos].hash_val_a == hash_val_a && hashtable[hash_pos].hash_val_b == hash_val_b)
			break;
		hash_pos = (hash_pos + 1) % hashtable_cur_len;
		if(hash_pos == hash_start)
			return -1;
	}

	return hash_pos;
}

int hash_add( unsigned char *key, int key_len, void *value )
{
	unsigned int hash_val, hash_val_a, hash_val_b;
	unsigned int hash_start;
	unsigned int hash_pos;

	hash_val = hash_charset(key, key_len, HASH_OFFSET);
	hash_val_a = hash_charset(key, key_len, HASH_A);
	hash_val_b = hash_charset(key, key_len, HASH_B);
	hash_start = hash_val % hashtable_cur_len;

	//if the buf is full, realloc
    hash_pos = get_add_hash_pos(hash_start,hash_val_a,hash_val_b);
	if(hash_pos == -1)
	{
		if(hash_alloc() == 0)
			hash_pos = get_add_hash_pos(hash_start,hash_val_a,hash_val_b);
		if (hash_pos == -1)
			return -1;
	}

	hashtable[hash_pos].hash_val_a = hash_val_a;
	hashtable[hash_pos].hash_val_b = hash_val_b;
	hashtable[hash_pos].exists = 1;
	hashtable[hash_pos].key_len = key_len;
	hashtable[hash_pos].value = value;
	return 0;
}

int hash_get_charset_pos( unsigned char *charset, int charset_len )
{
	unsigned int hash_val, hash_val_a, hash_val_b;
	unsigned int hash_start;
	unsigned int hash_pos;

	hash_val = hash_charset(charset, charset_len, HASH_OFFSET);
	hash_val_a = hash_charset(charset, charset_len, HASH_A);
	hash_val_b = hash_charset(charset, charset_len, HASH_B);
	hash_start = hash_val % hashtable_cur_len;
	hash_pos = hash_start;

	while ( hashtable[hash_pos].exists)
	{
		if (hashtable[hash_pos].hash_val_a == hash_val_a && hashtable[hash_pos].hash_val_b == hash_val_b)
			return hash_pos;
		else
			hash_pos = (hash_pos + 1) % hashtable_cur_len;
		if (hash_pos == hash_start)
			break;
	}
	return -1;
}

void * hash_search( unsigned char *key, int key_len )
{
	unsigned int hash_pos;

	hash_pos = hash_get_charset_pos( key, key_len );
	if ( hash_pos == -1 )
		return NULL;
	return hashtable[hash_pos].value;
}

int hash_del( unsigned char *key, int key_len )
{
	unsigned int hash_pos;

	hash_pos = hash_get_charset_pos(key, key_len);
	if(hash_pos == -1)
		return -1;
	hashtable[hash_pos].hash_val_a = -1;
	hashtable[hash_pos].hash_val_a = -1;
	hashtable[hash_pos].exists = 0;
	hashtable[hash_pos].key_len = 0;
	hashtable[hash_pos].value = NULL;
	return 0;
}

void hash_destroy( hash_ex_func free_data )
{
	unsigned int i = 0;
	if(hashtable != NULL)
	{
		for( i=0; i< hashtable_cur_len; i++ )
		{
			if( NULL != hashtable[i].value )
			{
				if( (NULL!=free_data)&&(NULL!=hashtable[i].value) )
					free_data( hashtable[i].value );
				hashtable[i].value = NULL;
			}
		}
		free(hashtable);
		hashtable = NULL;
	}
}

int hash_visit( hash_ex_func_ex set_flags, void *pflags )
{
	unsigned int i = 0;
	if( NULL == hashtable )
		return -1;
	for( i=0; i< hashtable_cur_len; i++ )
	{
		if( (NULL!=hashtable[i].value)&&(NULL!=pflags) )
		{
			set_flags( hashtable[i].value, pflags );
		}
	}
	return 0;
}
