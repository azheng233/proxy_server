#ifndef  __SORT_REQUEST_H__
#define  __SORT_REQUEST_H__

typedef struct request_start {
        char method[10];
        char host[4096];
	char resource[4096];
        char version[100];
}start;

typedef struct request_first {
        char *key;
        char *value;
        struct request_first *next;
}first;

typedef struct hash_table {
        first *index[10];
}table;


first *find_hash(table *hash, char *find_data);

void print_hash(table *hash);

int judge_conn_method(table *hash);

table *create_hashtable();

int startline(int connect_fd, start **p);

void destory_hash(table *hash);

int get_head(int connect_fd, table **head, SSL *server_ssl);


#endif

