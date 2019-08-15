#ifndef linklist_H_
#define linklist_H_

struct link {
	int num;
	struct link *next;
};

extern struct link *create(void);

extern int link_length(struct link *head);

extern struct link *insert(struct link *head, int place, int new_num);

extern void delete(struct link *head, int place_del);

extern void destroy(struct link *head);

extern void print(struct link *head);

extern int locate(struct link *head, int e);

#endif
