#ifndef __HTTP_H_
#define __HTTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define PORT 8000
#define MAXLINE 100
#define BUFLEN 1024
#define BUCKET 10

struct link {
        char str[4096];
        struct link *next;
};

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

void conn(char *buff_new, char *buff_one, char *p);
struct link *create(void);
void print(struct link *head);
int find_return (char *buf);
struct link *resolve(char *buf, char *symbol);
static char *recv_link (int connect_fd);
struct link *receive(int connect_fd, char *symbol);

int ELF_hash(char *cmd);
first *find_hash (table *hash, char *find_data);
void print_hash (table *hash);
start *get_startline (int connect_fd);
table *get_first (int connect_fd);

void SERROR (char *xyfirst, int connect_fd, int a);
int SFILE (char *xyfirst, int connect_fd, FILE *fd, char *URL);

#endif
