#include "https.h"
#include "sort_request.h"
#include "recvline.h"
#include "client.h"

char *edit_message(start *line1, table *hash)
{
	int n, len, i;
	first *tmp;
	char *message;
	message = malloc(4096);

	n = sprintf(message, "%s %s %s\r\nHost: %s\r\n", 
		line1->method, line1->resource, line1->version, 
		line1->host);
	len = n;
	
	for (i = 0; i < BUCKET; i++) {
		tmp = hash->index[i];

		while (tmp != NULL) {
			if (strcmp(tmp->key, "Host") == 0) {
				tmp = tmp->next;
				continue;
			} else if (strcmp(tmp->key, "Proxy-Connection") == 0) {
				tmp = tmp->next;
				continue;
			} else if (strcmp(tmp->key, "Connection") == 0) {
				tmp = tmp->next;
				continue;
			}

			n = sprintf(message + len, "%s: %s\r\n", tmp->key, tmp->value);
			len += n;

			tmp = tmp->next;
		}
	}
	n = sprintf(message + len, "Connection: close\r\n\r\n");

	return message;
}

int send_request_head(int client_socket, start *line1, table *hash)
{
	char *message;
	int err;	

	message = edit_message(line1, hash);
	err = send(client_socket, message, strlen(message), MSG_NOSIGNAL);
	if (err < 0) {
		fprintf(stderr, "send error: %m\n");
		return -1;
	}

	free(message);
	message = NULL;

	return 0;
}

static int forward_response_message(int client_socket, int connect_fd)
{
	char *buf;
	int err = 0, r_num, s_num;
	int i=0;
	
	buf = malloc(4096);
	if (buf == NULL) {
		return -1;
	}
	
	while (1) {
		r_num = recv(client_socket, buf, 4095, 0);
		if (r_num < 0 && errno == EAGAIN) {
			fprintf(stderr, "client.c recv timeout\n");
			break;
		} else if (r_num < 0) {
			fprintf(stderr,"recv error: %m(error: %d)\n", errno);
			err = r_num;
			break;
		} else if (r_num == 0) {
			fprintf(stderr, "forward over\n");
			break;
		}
		fprintf(stderr, "<");

		buf[r_num] = '\0';

		s_num = send(connect_fd, buf, r_num, MSG_NOSIGNAL);
		if (s_num < 0 && errno != SIGPIPE) {
			fprintf(stderr, "send error: %m (error: %d)\n", errno);
			err = s_num;
			break;
		}
		fprintf(stderr, ">");
	}
	free(buf);
	buf = NULL;

	return err;
}

static int forward_request_entity(int connect_fd, int len, int client_socket)
{
	int rnum, snum;
	char entity[4096];
	char *tail;

	if ((tail = receive_tail()) != NULL) {
		snum = send(client_socket, tail, strlen(tail), MSG_NOSIGNAL);
		len -= strlen(tail);
		free(tail);
		tail = NULL;
	}

	while (len > 0) {
		rnum = recv(connect_fd ,entity, 4095, 0);
		if (rnum <= 0) {
			fprintf(stderr, "recv request entity fail\n");
			return -1;
		}
		
		entity[rnum] = '\0';
		
		fprintf(stderr, "entity: %s\n", entity);
		snum = send(client_socket, entity, rnum, MSG_NOSIGNAL);
		if (snum <= 0) {
			fprintf(stderr, "send request entity fail\n");
			return -1;
		}
		
		len -= rnum;
	}

	return 0;
}

int get_entity_len(table *hash)
{
	int len = 0, err = 0;
	first *accept_len, *content_len;
	accept_len = find_hash(hash, "Accept-Length");
	if (accept_len == NULL) {
		fprintf(stderr, "NO find the Accept-Length\n");
		len = 0;
	} else {
		len = atoi(accept_len->value);
		if (len) {
			fprintf(stderr, "The entity length is %d\n", len);
			return len;
		}
	}
	
	content_len = find_hash(hash, "Content-Length");
	if (content_len == NULL) {
		fprintf(stderr, "NO find the Content-Length\n");
		len = 0;
	} else {
		len = atoi(content_len->value);
		if (len) {
			fprintf(stderr, "The entity length is %d\n", len);
		}
	}

	return len;
}

static int send_request_entity(int connect_fd, int client_socket, table *hash)
{
	int err, len;
	
	if (len) {
		err = forward_request_entity(connect_fd, len, client_socket);
	} else {
		err = 0;
	}
	
	return err;
}


int process(int connect_fd, int client_socket, start *line1, table *hash)
{
	int err;

	err = send_request_head(client_socket, line1, hash);
	if (err) 
		return err;
	
	err = send_request_entity(connect_fd, client_socket, hash);
	if (err)
		return err;

	err = forward_response_message(client_socket, connect_fd);
	
	return err;
}

int http_client(char *ip, char *port, int connect_fd, start *line1, table *hash)
{
	int client_socket, addrlen, listen_socket;
	int err = 0;
        struct sockaddr_in addr;
	int port1;

	port1 = atoi(port);
	
	client_socket = socket(AF_INET, SOCK_STREAM, 0);   //创建和服务器连接套接字  
        if(client_socket == -1) {
                perror("Client socket error:");
                return -1;
        }

        memset(&addr, 0, sizeof(addr));

        addr.sin_family = AF_INET;  /* Internet地址族 */
        addr.sin_port = htons(port1);  /* 端口号 */
        addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* IP地址 */
        inet_aton(ip, &(addr.sin_addr));

        addrlen = sizeof(addr);

        listen_socket =  connect(client_socket,  (struct sockaddr *)&addr, addrlen);
        if(listen_socket == -1) {
                fprintf(stderr,"Connect %s:%d error: %m\n", ip, port1);
		goto end;
	}
	struct timeval timeout = {1,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, 
		(char *)&timeout, sizeof(struct timeval));
        if (err) {
                fprintf(stderr, "timeout error :%m(error: %d\n)", errno);
        }

	err = process(connect_fd, client_socket, line1, hash);
end:
	close(client_socket); 	
	return err;
}
