#include "http.h"

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        head->next = NULL;
	strcpy(head->str, "0");
        return head;
}

/*打印函数*/
void print(struct link *head)
{
        struct link *p;

        p = head->next;
        while (p != NULL) {
                printf("%s ", p->str);
                p = p->next;
        }
	printf("\n");
}

int find_return (char *buf)
{
        int i = 0;

        while (buf[i] != '\n') {
                if (buf[i] == '\0') {
                        return 0;
                }

                i++;
        }
        return i+1;
}

struct link *resolve(char *buf, char *symbol) 
{
	char node[200];
	struct link *data, *p1, *p2;
	int i = 0;
	int j = 0;

	data = create();
	p1 = create();
	if (data == NULL || p1 == NULL) {
		printf("create error\n");
	}

	while (buf[i] != symbol[0] && buf[i] != '\r') {
		node[0] = buf[i];
		node[1] = '\0';

		++i;
		j = 0;
		
		while (1) {
			j += 1;

			if (buf[i] == symbol[0] && symbol[0] == ' ') {
				i++;
				break;
			}
			
			if (buf[i] == ':' && buf[i+1] == ' ') { 
				i += 2;
				break;
			}
			
			if (buf[i] == '\r') {
				break;
			}
				
				
			node[j] = buf[i]; 	
			node[j + 1] = '\0'; 	

			++i;
		}

		if (data->next == NULL)
			data->next = p1;
		else 
			p2->next = p1;

		strcpy(p1->str, node);
		p2 = p1;
		if (buf[i] != '\n') {
			p1 = create();
		}
	}		
	p2->next = NULL;
	return data;
}

static char *recv_link (int connect_fd) 
{
	static char *tail;
	static int tail_len = 0;
	char *buf;
	int buflen = 0, len, return_pos;
	int size = 50;

	buf = malloc(size);
	buf[0] = '\0';

	while (1) {
		if (buflen >= size) {
			size += 50;
			buf = (char *)realloc(buf, size);
		}
	
		if (tail) {
			strncpy(buf, tail, tail_len);
			buf[tail_len] = '\0';

			return_pos = find_return(buf);
printf("return = %d\n", return_pos);
			if (return_pos > 0) {
				tail_len = tail_len - return_pos;
				if (tail_len) {
					tail = malloc(tail_len + 1);
					strncpy(tail, buf + return_pos, tail_len);
					tail[tail_len] = '\0';
				}
				buf[return_pos] = '\0';
				if (buf[return_pos - 2] != '\r') {
					buf[0] = '\n'; buf[1] = '\0';
					return buf;	
				} else {
					return buf;
				}
				
			} else {
				buflen = tail_len;
			}
		}
printf("A-\n");
		len = recv(connect_fd, buf + buflen, size - buflen, 0);
printf("-A\n");
printf("len = %d\n", len);
		if (len > 0) {

			buflen += len;
		} else if (len == 0) {
			printf("disconnection\n");
			buf[0] = '\0';
			return buf;
		} else if (len < 0 && errno != EAGAIN) {
			printf("recv error:%m");

			return buf = NULL	
		} else {
			free(tail);
			tail = NULL;
			buf[0] = '\n'; buf[1] = '\0';
			return buf;	
		}
		
		buf = (char *)realloc(buf, buflen + 1);
		buf[buflen] = '\0';
printf("tail + buf:%s\n", buf);
		
		tail = malloc(buflen + 1);
		sprintf(tail, buf);	
		tail_len = buflen;
printf("tail:%s\n", tail);
	}

}

int receive (int connect_fd, char *symbol, struct link *data)
{
		
	char *buf;
	
	buf = recv_link(connect_fd);
	if (buf == NULL) {
		return 500;
	}
	
	if (buf[0] == '\0') {
		return -1;
	} else if (buf[0] == '\n') {
		return 400
	} else if (buf[0] == '\r') {
		data = create();	
	} else {
		data = resolve(buf, symbol);
	}
	
	return 0;
}
