#include "http.h"

int ELF_hash(char *cmd)
{
	unsigned int hash=0;  
	unsigned int x=0;  
	while(*cmd)  
	{  
	    	hash=(hash<<4)+*cmd;  
	    	if((x=hash & 0xf0000000)!=0)  
	   	{  
	        	hash^=(x>>24);    
	        	hash&=~x;  
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

start *get_startline (int connect_fd)
{
	struct link *data;
	int n, i, l;
	char symbol[2];
	start *line1;
	
	symbol[0] = ' ';
	symbol[1] = '\0';
	
printf("================================================\n");	
	data = receive(connect_fd, symbol);
	if (data == NULL)
		return line1 = NULL;

	line1 = malloc(sizeof(start));
	data = data->next;
	strcpy(line1->method, data->str);
	
	data = data->next;
	strcpy(line1->url, data->str);

	data = data->next;
	strcpy(line1->version, data->str);
	
	return line1;
}


table *get_first (int connect_fd) 
{
	struct link *data;
	int n, i;
	first *entry, *front;
	char symbol[2];
	table *hash;

	symbol[0] = ':';
	symbol[1] = '\0';

	hash = malloc(sizeof(first)*BUCKET);
	for (n = 0; n < 10; n++) {
		hash->index[n] = NULL;
	}

	while (1) {
		data = receive(connect_fd, symbol);
		if (data == NULL)
			return hash = NULL;

		if (0 == strcmp(data->str, "\n")) {
			return hash;
		}

		if (data->next == NULL) {
			return hash;
		}

		entry = malloc(sizeof(first));	
		entry->next = NULL;

		data = data->next;
		entry->key = malloc(strlen(data->str));
		strcpy(entry->key, data->str);

		data = data->next;
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
	}
}

