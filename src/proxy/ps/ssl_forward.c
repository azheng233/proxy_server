#include "https.h"
#include "ssl_handle.h"

char *edit_ssl_message(char *ssl_line1, table *ssl_head)
{
	int n, len, i;
	char *message;
	first *tmp;
	
	message = malloc(4096);
	n = sprintf(message, "%s", ssl_line1);
	len = n;

	for (i=0; i < BUCKET; i++) {
		tmp = ssl_head->index[i];

		while (tmp != NULL) {
			if(strcmp(tmp->key, "Connection") == 0) {
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

int ssl_forward_request_head(SSL *client_ssl, char *ssl_line1, table *ssl_head)
{
	int err;	
	char *message;

	message = edit_ssl_message(ssl_line1, ssl_head);
fprintf(stderr, "message:\n%s\n", message);
	err = SSL_write(client_ssl, message, 4096);
	if (err == -1) {
		ERR_print_errors_fp(stderr);
	} else {
		err = 0;
	}

	free(message);
	message = NULL;

	return err;
}

int ssl_forward_request_entity(SSL *client_ssl, SSL *server_ssl, table *ssl_hash)
{
	int len;
	int err;
	int rnum, wnum;
	char entity[4096];
	char *tail;

	len = get_entity_len(ssl_hash);
	if (!len) {
		fprintf(stderr, "No entity\n");
		return 0;
	}

	if ((tail = receive_tail()) != NULL) {
		wnum = SSL_write(client_ssl, tail, strlen(tail));
		len -= strlen(tail);
		free(tail);
		tail = NULL;
	}

	while (len > 0) {
		rnum = SSL_read(server_ssl ,entity, 4095);
		if (rnum <= 0) {
			ERR_print_errors_fp(stderr);
			return -1;
		}
		entity[rnum] = '\0'; 
		fprintf(stderr, "entity: %s\n", entity); 
		wnum = SSL_write(client_ssl, entity, rnum); 
		if (wnum <= 0) { 
			ERR_print_errors_fp(stderr);
			return -1; 
		} 
		len -= rnum;
	}

	return 0;
}

int ssl_forward_response_message(SSL *server_ssl, SSL *client_ssl)
{
	char *buf;
	int err = 0, r_num, w_num;
	int i=0;
	
	buf = malloc(4096);
	if (buf == NULL) {
		return -1;
	}
	
	while (1) {
		r_num = SSL_read(client_ssl, buf, 4095);
		if (r_num < 0 && errno == EAGAIN) {
			fprintf(stderr, "client.c recv timeout\n");
			break;
		} else if (r_num < 0) {
			ERR_print_errors_fp(stderr);
			err = r_num;
			break;
		} else if (r_num == 0) {
			fprintf(stderr, "forward over\n");
			break;
		}
		fprintf(stderr, "<");

		buf[r_num] = '\0';

		w_num = SSL_write(server_ssl, buf, r_num);
		if (w_num < 0 && errno != SIGPIPE) {
			ERR_print_errors_fp(stderr);
			err = w_num;
			break;
		}
		fprintf(stderr, ">");
	}
	free(buf);
	buf = NULL;

	return err;
}

int SSL_forward(SSL *server_ssl, SSL *client_ssl, start *line1, struct link *whitelist)
{
	int err = 0;
	char *ssl_line1;
	table *ssl_head;

        ssl_head = create_hashtable(ssl_head);
        if (ssl_head == NULL) {
		err = -1;
		goto send_err;
        }

	ssl_line1 = sslread_line(server_ssl);
	if (ssl_line1 == NULL || ssl_line1[0] == '\1') {
		err = -1;
		goto send_err;
	} else if (ssl_line1[0] == '\0') {
		err = 1;
		goto send_err;
	}
	err = get_head(0, &ssl_head, server_ssl);
	if (err)
		goto send_err;

	if (judge_url(line1, whitelist, ssl_line1) == 0) {
                fprintf(stderr,"This request is not on the white list.\n");
		return 1;
	}
	err = ssl_forward_request_head(client_ssl, ssl_line1, ssl_head);
	if (err == -1)
		goto send_err;

	err = ssl_forward_request_entity(client_ssl, server_ssl, ssl_head);
	if (err)
		goto send_err;
	err = ssl_forward_response_message(server_ssl, client_ssl);
		
send_err:
	if (ssl_line1) {
		free(ssl_line1);
		ssl_line1 = NULL;
	}
	if (ssl_head) {
		destory_hash(ssl_head);
	}
	return err;
}
