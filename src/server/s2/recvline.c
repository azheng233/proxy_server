#include "http.h"
#include "recvline.h"
#include "edit_message.h"

static struct link *create(void)
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

void destroy(struct link *data)
{
	struct link *p;

	while (data) {
		p = data->next;
		free(p);
		p = NULL;
		data = p;
	}
}

static int find_return (char *buf)
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

static struct link *resolve(char *buf, char *symbol) 
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

static char *recv_line (int connect_fd) 
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
			if (return_pos > 1 && buf[return_pos - 2] != '\r') {
				printf("Format is wrong\n");
				free(tail);
				tail =  NULL;
				return buf = NULL;
			}
			
			if (return_pos > 0) {
				tail_len = tail_len - return_pos;
				if (tail_len) {
					tail = malloc(tail_len + 1);
					strncpy(tail, buf + return_pos, tail_len);
					tail[tail_len] = '\0';
				}

				buf[return_pos] = '\0';
printf("buf:%s", buf);
				return buf;
				
			} else {
				buflen = tail_len;
			}
		}
		len = recv(connect_fd, buf + buflen, size - buflen, 0);
		if (len > 0) {
			buflen += len;
	
		} else if (len == 0) {
			printf("disconnection\n");
			sprintf(buf, "\0");					

			return buf;	
		} else if (len < 0 && errno != EAGAIN) {
			printf("recv fail\n");
			printf("recv error:%m");
			buf[0] = '\1';

			return buf;	
		} else if (len < 0 && errno == EAGAIN){
			printf("recv timeout\n");
			free(tail);
			tail = NULL;

			return buf = NULL;
		}
		
		buf = (char *)realloc(buf, buflen + 1);
		buf[buflen] = '\0';
		
		tail = malloc(buflen + 1);
		sprintf(tail, buf);	
		tail_len = buflen;
	}

}

struct link *receive_line(int connect_fd, char *symbol)
{
	struct link *data;
	char *buf;
	
	buf = recv_line(connect_fd);
	if (buf == NULL) {
		data = NULL;
		return data;
	}	

	if (buf[0] == '\0') {
		sprintf(data->str,"\0");
	} else if (buf[0] == '\1') {
		data = create();	
		sprintf(data->str, "\1");
	} else if (buf[0] == '\r') {
		data = create();	
		sprintf(data->str, "\r");
	} else {
		data = resolve(buf, symbol);
	}

	return data;
}
//17点23分
