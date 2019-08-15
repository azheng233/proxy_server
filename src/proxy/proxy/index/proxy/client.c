#include "http.h"
#include "sort_request.h"
#include "client.h"

static char *edit_message(start *line1, table *hash)
{
	char *message;
	int n, len, i;
	first *tmp;
	
	message = malloc(1024);

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
			}

			n = sprintf(message + len, "%s: %s\r\n", tmp->key, tmp->value);
			len += n;

			tmp = tmp->next;
		}
	}
	
	n = sprintf(message + len, "%s", "\r\n");

	return message;
}

static int forward(int client_socket, int connect_fd)
{
	char *buf;
	int err = 0, r_num, s_num;
	
	struct timeval timeout = {1,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	
	buf = malloc(1000);
	if (buf == NULL) {
		return -1;
	}

        if (err) {
                printf("timeout error :%m(error: %d\n)", errno);
        }
	
	while (1) {
		
		r_num = recv(client_socket, buf, 999, 0);
		if (r_num < 0 && errno == EAGAIN) {
			printf("client.c recv timeout\n");
			break;
		} else if (r_num < 0) {
			printf("recv error: %m(error: %d)\n", errno);
			err = r_num;
			break;
		} else if (r_num == 0) {
			printf("forward over\n");
			break;
		}
		buf[r_num] = '\0';
		printf(".");
		s_num = send(connect_fd, buf, r_num, 0);
		if (s_num < 0) {
			printf("send error: %m\n");
			err = s_num;
			break;
		}
	}
	free(buf);
	buf = NULL;

	return err;
}

int client(char *ip, char *port, int connect_fd, start *line1, table *hash)
{
	int client_socket, addrlen, listen_socket;
	int err;
        struct sockaddr_in addr;
	char *message;
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
        if(listen_socket == -1)
        {
                perror("Connect error");
                return -1;
        }

        printf("成功连接到目标服务器\n");

	message = edit_message(line1, hash);
//	printf("Message:\n%s",message);
	err = send(client_socket, message, strlen(message), 0);
	if (err < 0) {
		printf("send error: %m\n");
		return -1;
	}
	
	free(message);
	message = NULL;

	err = forward(client_socket, connect_fd);
	
	close(client_socket); 	
	return 0;
}
