#ifndef  __EDIT_MESSAGE_H__
#define  __EDIT_MESSAGE_H__

typedef struct request_start {
        char method[10];
        char url[100];
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

start *get_startline(struct link *data, start *line1);

table *get_first(struct link *data, table *hash);

char *getboby(table *hash, int connect_fd);

#endif

