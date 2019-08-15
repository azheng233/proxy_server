#ifndef __JUDGE_REQUEST_H_
#define __JUDGE_REQUEST_H_

int judge_line1(start *line1, char **target_ip, char **target_port);

int judge_conn_method(table *hash);

int judge_url(start *line1, struct link *whitelist, char *ssl_line1);

#endif
