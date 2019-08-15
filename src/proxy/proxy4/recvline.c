#include "recvline.h"

static char *tail;
static int tail_len = 0;

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof(struct link));
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
                fprintf(stderr, "%s ", p->str);
                p = p->next;
        }
	fprintf(stderr, "\n");
}

void destory(struct link *data)
{
	struct link *p;

	while (data) {
		p = data->next;
		free(data);
		data = p;
	}
}

int find_word (char *buf, char *word)
{
        int i = 0;

        while (buf[i] != word[0]) {
                if (buf[i] == '\0') {
                        return 0;
                }

                i++;
        }
        return i+1;
}

struct link *resolve(char *buf, char *symbol)
{
	char node[4096];
	struct link *data, *p1, *p2;
	int j = 0;
	int i = 0;
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
		p1->str[j] = '\0';
		p2 = p1;
		if (buf[i] != '\r') {
			p1 = create();
		}
	}
	p2->next = NULL;
	return data;
}

static char *recv_line (int connect_fd)
{
	char *buf;
	int buflen = 0, len, return_pos;
	int size = 100;

	buf = (char *)malloc(size + 1);
	buf[0] = '\0';

	while (1) {
		if (buflen >= size) {
			size += 50;
			buf = (char *)realloc(buf, size + 1);
		}

		if (tail) {
			strncpy(buf, tail, tail_len);
			buf[tail_len] = '\0';
			
			free(tail);
			tail =  NULL;

			return_pos = find_word(buf, "\n");
			if (return_pos > 1 && buf[return_pos - 2] != '\r') {
				fprintf(stderr, "Format is wrong\n");
				free(buf);
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
				fprintf(stderr, "%s", buf);
				return buf;
f
			} else {
				buflen = tail_len;
			}
		}

		len = recv(connect_fd, buf + buflen, size - buflen, 0);

		if (len > 0) {
			buflen += len;

		} else if (len == 0) {
			fprintf(stderr, "proxy disconnection\n");
			buf[0] = '\0';

			if (tail) {
				free(tail);
				tail = NULL;
			}
			return buf;
		} else if (len < 0 && errno != EAGAIN) {
			fprintf(stderr, "recv fail: %m\n");
			buf[0] = '\1';
			
			if (tail) {
				free(tail);
				tail = NULL;
			}

			return buf;
		} else if (len < 0 && errno == EAGAIN){
			fprintf(stderr, "proxy recv timeout\n");
			if (tail) {
				free(tail);
				tail = NULL;
			}
			free(buf);
			buf = NULL;
			return buf;
		}

		buf[buflen] = '\0';
		tail = malloc(buflen + 1);
		sprintf(tail, "%s", buf);
		tail_len = buflen;
	}

}

char *sslread_line (SSL *server_ssl)
{
	char *buf;
	int buflen = 0, len = 0, return_pos;
	int size = 100;
	size_t recvlen;

	buf = (char *)malloc(size + 1);
	buf[0] = '\0';

	while (1) {
		if (buflen >= size) {
			size += 50;
			buf = (char *)realloc(buf, size + 1);
		}

		if (tail) {
			strncpy(buf, tail, tail_len);
			buf[tail_len] = '\0';
			
			free(tail);
			tail =  NULL;

			return_pos = find_word(buf, "\n");
			if (return_pos > 1 && buf[return_pos - 2] != '\r') {
				fprintf(stderr, "Format is wrong\n");
				free(buf);
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
				fprintf(stderr, "%s", buf);
				return buf;

			} else {
				buflen = tail_len;
			}
		}

		len = SSL_read(server_ssl, buf + buflen, size - buflen);

		if (len > 0) {
			buflen += len;

		} else if (len == 0) {
			fprintf(stderr, "proxy disconnection\n");
			buf[0] = '\0';

			if (tail) {
				free(tail);
				tail = NULL;
			}
			return buf;
		} else if (len < 0 && errno != EAGAIN) {
			ERR_print_errors_fp(stderr);
			buf[0] = '\1';
			
			if (tail) {
				free(tail);
				tail = NULL;
			}

			return buf;
		} else if (len < 0 && errno == EAGAIN){
			if (tail) {
				free(tail);
				tail = NULL;
			}
			free(buf);
			buf = NULL;
			return buf;
		}

		buf[buflen] = '\0';
		tail = malloc(buflen + 1);
		sprintf(tail, "%s", buf);
		tail_len = buflen;
	}

}

char *receive_tail()
{
	char *entity;

	entity = malloc(strlen(tail) + 1);
	
	if (tail) {
		fprintf(stderr, "taillen: %d tail: %s\n", strlen(tail), tail);
		strcpy(entity, tail);
		entity[strlen(tail)] = '\0';
		
		free(tail);
		tail =NULL;
	}

	return entity;
}

struct link *receive_line(int connect_fd, char *symbol, SSL *server_ssl)
{
	struct link *data;
	char *buf;
	if (server_ssl)
		buf = sslread_line(server_ssl);
	else
		buf = recv_line(connect_fd);

	if (buf == NULL) {
		return NULL;
	}

	if (buf[0] == '\0') {
		data = create();
		data->str[0] = '\0';
	} else if (buf[0] == '\1') {
		data = create();
		data->str[0] = '\1';
		data->str[1] = '\0';
	} else if (buf[0] == '\r') {
		data = create();
		data->str[0] = '\r';
		data->str[1] = '\0';
	} else {
		data = resolve(buf, symbol);
	}
	if (buf) {
		free(buf);
		buf = NULL;
	}
	return data;
}
