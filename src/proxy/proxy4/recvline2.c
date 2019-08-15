#include "recvline.h"

enum RETURN_ERR {
	OK = 0;
	PROXY_UNABLE_RESPONSE = -1,	//服务器出现未知错误，不能通信。
	PROXY_RUN_ERROR = 500,		//服务器出现未知错误，不能正常代理。
	TARGET_DISCONNECTION = -2,	//客户端或源服务器中途断开连接。
	TARGET_TIMEOUT = -3,		//客户端或源服务器交互数据超时。
	MASSAGE_FORMAT_ERROR = 400,	//报文格式错误。
	MASSAGE_URL_ERROR = 404,	//报文url错误。
	NO_ON_THE_WHITELIST = 403	//URL不在白名单内。
};

enum RETURN_ERROR ERR_CODE;

int return_err() 
{
	return ERR_CODE;
}

int find_char(char *buf, char *word) 
{
	int i = 0;

	while (buf[i] != word[0]) {
		if (buf[i] == '\0')
			return 0;
		i++;
	}

	return i+1;
}

char *recv_request_header(int connect_fd)
{
	char *buf;
	int size = 512;
	int buflen = 0, len;

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
				ERR_CODE = MASSAGE_FORMAT_ERROR;
				goto err;
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
		
		len = recv(connect_fd, buf + buflen, size - buflen, 0);
		if (len > 0) {
			buflen += len;
		} else if (!len) {
			fprintf(stderr, "client disconnection\n");
			ERR_CODE = TARGET_DISCONNECT;
			goto err;
		} else if (len < 0 && errno != EAGAIN) {
			fprintf(stderr, "recv fail: %m\n");
			ERR_CODE = PROXY_RUN_ERROR;
			goto err;
		} else if (len < 0 && errno == EAGAIN) {
			fprintf(stderr, "recv timeout\n");
			ERR_CODE = MASSAGE_FORMAT_ERROR;
			goto err;
		}

		buf[buflen] = '\0';
		tail = malloc(buflen + 1);
		sprintf(tail, "%s", buf);
		tail_len = buflen;
	}
err:
	if (tail) {
		free(tail);
		tail = NULL;
	}
	free(buf);
	fprintf(stderr, "ERR_CODE: %d\n", ERR_CODE);
	return (buf = NULL);
}



start *startline1(char *buf, start *line1)
{
	int i, err;
	char tmp[2048];
	for (i = 0; buf[i] != ' '; i++) {
		line1->method[i] = buf[i];
		if (buf[i] == '\0') {
			ERR_CODE = MASSAGE_FORMAT_ERROR;
			goto err;
		}
	}
	++i;
	for (i = 0; buf[i] != ' '; i++) {
		line1->url[i] = buf[i];
		if (buf[i] == '\0') {
			ERR_CODE = MASSAGE_FORMAT_ERROR;
			goto err;
		}
	}
	++i;

	for (i = 0; buf[i] != '\r'; i++) {
		line1->version[i] = buf[i];
		if (buf[i] == '\0') {
			ERR_CODE = MASSAGE_FORMAT_ERROR;
			goto err;
		}
	}
	
	s = strncmp("http://", line1->url, 7);
	if (!s) {
		err = sprintf(tmp, "%s", line1->url + 7);
	} else {
		err = sprintf(tmp, "%s", line1->url);
	}

	if (err < 0) {
		ERR_CODE = PROXY_RUN_ERROR;
		goto err;
	} 
	
	s = find_word(tmp, "/");
	if (s) {
		err = sprintf(line1->resource, "%s", tmp + (s - 1));
		tmp[s - 1] = '\0';
	} else {
		line1->resource[0] = "/";	
		line1->resource[1] = "\0";	
	}
	
	char Port[10];
	s = find_word(tmp, ":");
	if (s) {
		err = sprintf(Port, "%s", tmp + (s - 1));
		tmp[s - 1] = '\0';
	} else {
		if (strcmp(line1->method, "CONNECT"))
			line1->port = 443;
		else 
			line1->port = 80;
	}

	err = sprintf(line1->dns, "%s", tmp);
	if (err) {
		ERR_CODE = PROXY_RUN_ERROR;
		goto err;
	}


err:
	free(buf);
	buf = NULL;
	return line1;

}

int get_startline(int connect_fd, start **p)
{
	start *line1;
	char *buf;
	line1 = *p;

	buf = recv_request_header(connect_fd);
	if (!buf) {
		fprintf(stderr, "ERR_CODE: %d\n", ERR_CODE);
		return ERR_CODE;
	}

	line1 = startline1(buf, line1)
}
