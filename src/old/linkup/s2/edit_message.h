#ifndef  __EDIT_MESSAGE_H__
#define  __EDIT_MESSAGE_H__

typedef struct request_start start;
typedef struct request_first start;
typedef struct request_hash_table table;

first *find_hash (table *hash, char *find_data);
void print_hash (table *hash);
start *get_startline (struct link *data, start *line1);
table *get_first (struct link *data, table *hash);
char *getboby (table *hash, int connect_fd);

#endif

