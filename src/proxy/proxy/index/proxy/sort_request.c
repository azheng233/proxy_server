#include "http.h"
#include "sort_request.h"
#include "recvline.h"

start *get_line1 (struct link *data, start *line1)
{
	int n, err = 0;

	data = data->next;
	err = sprintf(line1->method, data->str);
	if (err == -1)
	    return line1 = NULL;

	data = data->next;
	err = strncmp("http://", data->str, 7);
	if (err == 0)
	    sprintf(data->str, data->str + 7);

	n = find_word(data->str, "/");
	if (n > 0) {
	    err = snprintf(line1->host, n, data->str);
	    if (err == -1)
	        return line1 = NULL;

	    err = sprintf(line1->resource, data->str + (n-1));
	    if (err == -1)
	        return line1 = NULL;
	} else {
	    err = sprintf(line1->host, data->str);
	    if (err == -1)
	        return line1 = NULL;
	    err = sprintf(line1->resource, "/");
	    if (err == -1)
	        return line1 = NULL;

	}

	data = data->next;

	strcpy(line1->version, data->str);
	return line1;
}

int startline(int connect_fd, start **p)
{
	int err = 0;
	struct link *data;
	start *line1;

	line1 = *p;

printf("********************\n");
        data = receive_line(connect_fd, " ");
printf("******************************************\n");
        if (data == NULL) {
                return 1;
        } else if (data->str[0] == '\0') {
                return 1;
        } else if (data->str[0] == '\1') {
                return 500;
        }

        line1 = get_line1(data, line1);
        if (line1 == NULL) {
                err = 400;
        }


	destroy(data);
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
		printf("Index[%d]:", i);

		q = hash->index[i];
		while (q != NULL) {
			printf("key[%s],value:[%s]->", q->key, q->value);
			q = q->next;
		}
		printf("NULL\n");
	}
}


table *insert_hash(struct link *data, table *hash)
{
	int i;
	first *entry, *front;

	entry = malloc(sizeof(first));
	entry->next = NULL;

	data = data->next;
	if (data == NULL)
		return hash = NULL;

	i = ELF_hash(data->str);

	entry->key = malloc(strlen(data->str));
	strcpy(entry->key, data->str);

	data = data->next;
	if (data == NULL)
		return hash = NULL;

	entry->value = malloc(strlen(data->str));
	strcpy(entry->value, data->str);

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

int del_hashnode(table *hash, char *del_key)
{
	int i, n;
	first *del_node, *front_node, *tmp;

	if (find_hash(hash, del_key) == NULL) {
	    printf("This node is not found.\n");
	    printf("Delete failed\n");
	    return -1;
	}

	n = ELF_hash(del_key);
	del_node = hash->index[i];

	n = strcmp(del_node->key, del_key);
	if (n == 0) {
        front_node = del_node;
        front_node = del_node->next;

        free(del_node);
        return;
	}

	while (del_node != NULL) {
        front_node = del_node;
        del_node = front_node->next;

        n = strcmp(tmp->key, del_key);
        if (n == 0) {
            front_node->next = del_node->next;
	        free(del_node);
	        break;
	    }
	}

	return 0;
}
/*
void destory_hash(table *hash)
{
	int i;
	first *node;

	for (i = 0; i < BUCKET; i++) {
		node = hash->index[i];
		while () {}
	}
}
*/
int get_head(int connect_fd, table **head)
{
	int err = 0;
	struct link *data;
	table *hash;

	hash = *head;

        while (1) {
                data = receive_line(connect_fd, ":");
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
                if (hash == NULL) {
                        err = 400;
                        break;
                }
        }

	destroy(data);
	return err;
}

char *getboby (table *hash, int connect_fd)
{
	int len;
	char *main_boby;
	first *accept_len;

	accept_len = find_hash(hash, "Accept-Length");
        if (accept_len) {
                len = atoi(accept_len->value);
                main_boby = malloc(len);
                recv(connect_fd, main_boby, len+1, 0);
                main_boby[len] = '\n';

        } else {
                printf("No main boby\n");
        }

	return main_boby;
}
