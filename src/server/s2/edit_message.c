#include "http.h"
#include "recvline.h"
#include "edit_message.h"

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
			return q;
		} else {
			if (q->next != NULL)
				q = q->next;
			else {
				q->key = NULL;
				q->value = NULL;
				q->next = NULL;	
				return q;
			}
		}
	}
}

void print_hash (table *hash) 
{
	first *q;
	int i;
	for (i = 0; i < BUCKET; i++) {
		printf("Index[%d]:", i);	
	
		q = hash->index[i];
		while (q != NULL) {

			printf("key(%s),value:(%s)->", q->key, q->value);
			q = q->next;
		}
		printf("NULL\n");
	}
}

start *get_startline (struct link *data, start *line1)
{
printf("================================================\n");	
	line1 = malloc(sizeof(start));
	data = data->next;
	if (data == NULL) 
		return line1 = NULL;

	strcpy(line1->method, data->str);
	
	data = data->next;
	if (data == NULL) 
		return line1 = NULL;

	strcpy(line1->url, data->str);

	data = data->next;
	if (data == NULL) 
		return line1 = NULL;

	strcpy(line1->version, data->str);
	
	return line1;
}


table *get_first (struct link *data, table *hash) 
{
	int i;
	first *entry, *front;

	entry = malloc(sizeof(first));	
	entry->next = NULL;

	data = data->next;
	if (data == NULL) 
		return hash = NULL;

	entry->key = malloc(strlen(data->str));
	strcpy(entry->key, data->str);

	data = data->next;
	if (data == NULL) 
		return hash = NULL;

	entry->value = malloc(strlen(data->str));
	strcpy(entry->value, data->str);
	i = ELF_hash(data->str);	
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
