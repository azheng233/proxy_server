#include "http.h"
#include "sort_request.h"
#include "recvline.h"

start *get_line1 (struct link *data, start *line1)
{
	int n, err = 0;
	char *url;
	struct link *p;
	
	p = data->next;
	err = sprintf(line1->method, "%s", p->str);
	if (err == -1)
	    	return line1 = NULL;

	p = p->next;
	err = strncmp("http://", p->str, 7);
	url = malloc(strlen(p->str) + 1);
	if (err == 0) {
		sprintf(url, "%s", p->str + 7);
	} else {
		sprintf(url, "%s", p->str);
	}

	n = find_word(url, "/");
	if (n > 0) {
	    	strncpy(line1->host, url, n-1);
		line1->host[n-1] = '\0';

	   	err = sprintf(line1->resource, "%s", url + (n-1));
	    	if (err == -1)
	        	return line1 = NULL;
	} else {
	    	err = sprintf(line1->host, "%s", url);
	    	if (err == -1)
	        	return line1 = NULL;
		err = sprintf(line1->resource, "/");
	    	if (err == -1)
	        	return line1 = NULL;
	}
	
	free(url);
	url = NULL;

	p = p->next;

	sprintf(line1->version, "%s", p->str);
	
	return line1;
}

int startline(int connect_fd, start **p)
{
	int err = 0;
	struct link *data;
	start *line1;
	char symbol[2];
	line1 = *p;
	
	symbol[0] = ' ';
	symbol[1] = '\0';
        data = receive_line(connect_fd, symbol);
        if (data == NULL) {
                return 1;
        } else if (data->str[0] == '\0') {
		destory(data);
                return 1;
        } else if (data->str[0] == '\1') {
		destory(data);
                return 500;
        }
        line1 = get_line1(data, line1);
        if (line1 == NULL) {
                err = 400;
        }

	destory(data);
	return 0;
}

table *create_hashtable(table *hash)
{
	int n;

	hash = malloc(sizeof(first) * BUCKET);

	for (n = 0; n < BUCKET; n++) {
		hash->index[n] = NULL;
	}

	return hash;
}

static int ELF_hash(char *cmd)
{
	unsigned int hash=0;
	unsigned int x=0;

	while (*cmd)
	{
	    	hash = (hash<<4) + *cmd;
	    	if((x=hash & 0xf0000000)!=0)
	   	{
	        	hash ^= (x >> 24);
	        	hash &= ~x;
	    	}
	    	cmd++;
	}

	return (hash & 0x7fffffff)%BUCKET;
}

first *find_hash (table *hash, char *find_data)
{
	first *q;
	int i, n;

	i = ELF_hash(find_data);

	q = hash->index[i];
	if (q == NULL) {
		return q;
	}

	while (1) {
		n = strcmp(q->key, find_data);
		if (n == 0) {
			break;
		} else {
			if (q->next != NULL)
				q = q->next;
			else {
				q = NULL;
				break;
			}
		}
	}

	return q;
}

void print_hash (table *hash)
{
	first *q;
	int i;
	for (i = 0; i < BUCKET; i++) {
		fprintf(stderr, "Index[%d]:", i);

		q = hash->index[i];
		while (q != NULL) {
			fprintf(stderr, "key[%s],value:[%s]->", q->key, q->value);
			q = q->next;
		}
		fprintf(stderr, "NULL\n");
	}
}


table *insert_hash(struct link *data, table *hash)
{
	int i;
	first *entry, *front;
	struct link *p;

	entry = malloc(sizeof(first));
	entry->next = NULL;

	p = data->next;

	i = ELF_hash(p->str);

	entry->key = malloc(strlen(p->str) + 1);
	sprintf(entry->key, "%s", p->str);

	p = p->next;
	entry->value = malloc(strlen(p->str) + 1);
	sprintf(entry->value, "%s", p->str);
	
	if (hash->index[i] == NULL) {
		hash->index[i] = entry;
	} else {
		front = hash->index[i];

	    	while (front->next != NULL) {
	    	    	front = front->next;
	    	}
	    	front->next = entry;
	}

	return hash;
}

void destory_hash(table *hash)
{
	int i;
	first *node, *delete;

	for (i = 0; i < BUCKET; i++) {
		delete = hash->index[i];
		while (delete != NULL) {
			node = delete->next;
			
			free(delete->key);
			delete->key = NULL;
			free(delete->value);
			delete->value = NULL;

			free(delete);
			delete = node;
		}
	}
	free(hash);
	hash = NULL;
}

int get_head(int connect_fd, table **head)
{
	int err = 0;
	struct link *data;
	table *hash;
	char symbol[2];
	hash = *head;

	symbol[0] = ':';
	symbol[1] = '\0';
        while (1) {
                data = receive_line(connect_fd, symbol);
                if (data == NULL) {
                        err = 1;
                        break;
                } else if (data->str[0] == '\0') {
                        err = 1;
                        break;
                } else if (data->str[0] == '\1') {
                        err = 500;
                        break;
                } else if (data->str[0] == '\r') {
                        break;
                }

                hash = insert_hash(data, hash);
		destory(data);
        }
	destory(data);
	return err;
}
