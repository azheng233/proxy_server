#include "http.h"
#include "recvline.h"
#include "sort_request.h"

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
		free(data);
		p = NULL;
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

static struct link *resolve(char *buf, char *symbol)
{
	char node[1024];
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
	int size = 100;

	buf = malloc(size);
	buf[0] = '\0';
	while (1) {
		if (buflen >= size) {
			size += 50;
			buf = (char *)realloc(buf, size + 1);
		}

		if (tail) {
			strncpy(buf, tail, tail_len);
			buf[tail_len] = '\0';

			return_pos = find_word(buf, "\n");
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
printf("buf:%s",buf);
				return buf;

			} else {
				buflen = tail_len;
			}
		}
		len = recv(connect_fd, buf + buflen, size - buflen, 0);

		if (len > 0) {
			buflen += len;

		} else if (len == 0) {
			printf("proxy disconnection\n");
			sprintf(buf, "\0");

			return buf;
		} else if (len < 0 && errno != EAGAIN) {
			printf("recv fail\n");
			printf("recv error:%m");
			buf[0] = '\1';

			return buf;
		} else if (len < 0 && errno == EAGAIN){
			printf("proxy recv timeout\n");
			if (tail) {
				free(tail);
				tail = NULL;
			}
			free(buf);
			buf = NULL;
			return NULL;
		}

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
		return NULL;
	}

	if (buf[0] == '\0') {
		data->str[0] = '\0';
	} else if (buf[0] == '\1') {
		data = create();
		sprintf(data->str, "\1");
	} else if (buf[0] == '\r') {
		data = create();
		sprintf(data->str, "\r");
	} else {
		data = resolve(buf, symbol);
	}

	free(buf);
	buf == NULL;
	return data;
}
