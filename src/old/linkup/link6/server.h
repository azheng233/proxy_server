#ifndef server_H_
#define server_H_

struct link {
        char num[4096];
        struct link *next;
};

char buff_one[4096], buff_new[4096], buff[4096];

struct link *create(void);

void conn(char *buff_one, char *buff_new, char *p);

char *receive(int connect_fd);

int link_length(struct link *head);

struct link *insert(struct link *head, int place, char *new_num);

void delete(struct link *head, int place_del);

void print(struct link *head);

int locate(struct link *head, char *e);

#endif
