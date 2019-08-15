#include "http.h"
#include "sort_request.h"
#include "judge_request.h"
#include "client.h"
#include "response_err.h"

int handle(int connect_fd)
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

printf("=================startline===================\n");
	err = startline(connect_fd, &line1);
	if (err)
		goto send_err;
	/*接收报文首部，成功返回0，出错返回相应错误代码*/
printf("=================get_head====================\n");
        head = create_hashtable(head);
        if (head == NULL) {
                return -1;
        }

	err = get_head(connect_fd, &head);
	if (err)
		goto send_err;

	/*判断URL是否可用，可以返回0，不可用返回错误代码*/
printf("=================judge_line==================\n");

	target_ip = malloc(46);
	target_port = malloc(10);

	/*判断URL是否可用，可以返回0，不可用返回错误代码*/
printf("Startline: %s,%s,%s,%s\n", line1->method,line1->host, line1->resource,line1->version);
	err = judge_line1(line1, &target_ip, &target_port);
	if (err)
		goto send_err;

	printf("IP:%s\nPORT:%s\n", target_ip, target_port);

printf("=================judge_connection============\n");
	/*keep-alive:n = 0 ,close: n = 1，*/
	n = judge_conn_method(head);
printf("=================client.c====================\n");

	n = client(target_ip, target_port, connect_fd, line1, head);

	send_err:
		if (err) {
printf("_________ERR = %d_________________", err);
			if (err != 1) {
printf("_________ERR1 = %d_________________", err);
				send_err(connect_fd, err);
				n = 1;
			} else {
				n = 1;
			}
		}

	free(line1);
	line1 = NULL;


	return n;
}
