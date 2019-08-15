#include "https.h"
#include "sort_request.h"
#include "recvline.h"
#include "judge_request.h"
#include "client.h"
#include "ssl_handle.h"
#include "response_err.h"

int handle(int connect_fd, SSL_CTX *server_ctx, SSL_CTX *client_ctx, struct link *whitelist)
{
	start *line1;
	table *head;
	first *conn_method;
	int n, err = 0;
	char *target_ip;
	char *target_port;
	/*接收报文起始行，成功返回0，出错返回相应错误代码*/
	
	line1 = malloc(sizeof(start));
	if (line1 == NULL) {
		err = -1;
		goto send_err;
	}

	target_ip = malloc(46);
	target_port = malloc(10);

        head = create_hashtable(head);
        if (head == NULL) {
		err = -1;
		goto send_err;
        }

	err = startline(connect_fd, &line1);
	if (err)
		goto send_err;
fprintf(stderr, "%s %s %s %s\n", line1->method, line1->host, line1->resource, line1->version);
	/*接收报文首部，成功返回0，出错返回相应错误代码*/

	err = get_head(connect_fd, &head, NULL);
	if (err)
		goto send_err;

	/*判断URL是否可用，可以返回0，不可用返回错误代码*/
	err = judge_line1(line1, &target_ip, &target_port);
	if (err)
		goto send_err;

	/*keep-alive:n = 0 ,close: n = 1，*/
	n = judge_conn_method(head);
	
	/*开始访问目标服务器并转发信息*/
	if (strcmp(line1->method, "CONNECT"))
		err = http_client(target_ip, target_port, connect_fd, line1, head, whitelist);
	else
		err = ssl_process(target_ip, target_port, connect_fd, line1, head, server_ctx, client_ctx, whitelist);

	send_err:
		if (err) {
			if (err != 1) {
fprintf(stderr, "_________ERR1 = %d_________________", err);
				send_err(connect_fd, err);
			}
			n = 1;
		}

	if (line1) {
		free(line1);
		line1 = NULL;
	}
	if (head) {
		destory_hash(head);
	}
	if (target_ip) {
		free(target_ip);
		target_ip = NULL;
	}
	if (target_port) {
		free(target_port);
		target_port = NULL;
	}

	return n;
}
