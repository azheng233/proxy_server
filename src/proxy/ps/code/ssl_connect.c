#include <openssl/ssl.h>
#include <openssl/err.h>
#include "http.h"
#include "sort_request.h"
#include "recvline.h"
#include "ssl_connect.h"
#include "client.h"

#define CAFILE "ssl/root_cert.pem"
#define CERTF "ssl/server_cert.pem"
#define	KEYF "ssl/key.pem"

static SSL_CTX *server_ctx;
static SSL_CTX *client_ctx;
static SSL *server_ssl;
static SSL *client_ssl;
static int client_socket;

static char *edit_message(start *line1, table *hash)
{
        char *message;
        int n, len, i;
        first *tmp;
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

static int send_request_head(start *line1, table *hash)
{
        char *message;
        int err;        

        message = edit_message(line1, hash);

        err = send(client_ssl, message, strlen(message), 0);
        if (err < 0) {
		fprintf(stderr, "send request head error: %m(%d)\n", errno);
                return -1;
        }

        free(message);
        message = NULL;

        return 0;
}

static int ssl_forward_response_message()
{
        char *buf;
        int err = 0, r_num, w_num;
        int i=0;
        
        buf = malloc(2048);
        if (buf == NULL) {
                return -1;
        }

        if (err) {
                fprintf(stderr, "timeout error :%m(error: %d\n)", errno);
        }
        
        while (1) {
                r_num = SSL_read(client_ssl, buf, 2047);
                if (r_num < 0 && errno == EAGAIN) {
                        fprintf(stderr, "client ssl_read respinse timeout\n");
                        break;
                } else if (r_num < 0) {
                        fprintf(stderr,"ssl read response error: ");
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
                        fprintf(stderr,"ssl write response error: ");
			ERR_print_errors_fp(stderr);
                        err = -1;
                        break;
                }
                fprintf(stderr, ">");
        }
        free(buf);
        buf = NULL;

        return err;
}

static int ssl_forward_request_entity(int len)
{
	int rnum, wnum;
	char entity[4096];
	char *tail;

	if ((tail = receive_tail()) != NULL) {
		wnum = SSL_write(client_ssl, tail, strlen(tail));
		len -= wnum;

		free(tail);
		tail = NULL;
	}

	while (len > 0) {
		rnum = SSL_read(server_ssl, entity, 4095);
		if (rnum <= 0) {
			fprintf(stderr, "ssl_read request entity error: ");
			ERR_print_errors_fp(stderr);
			return -1;
		}

		entity[rnum] = '\0';
		wnum = SSL_write(client_ssl, tail, strlen(tail));
		if (rnum <= 0) {
			fprintf(stderr, "ssl_wirte request entity error: ");
			ERR_print_errors_fp(stderr);
			return -1;
		}
		
		len -= rnum;
	}

	return 0;
}

static int get_entity_len(table *hash)
{
        int len, err = 0;
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

static int ssl_write_request_entity(table *hash)
{
	int err, len;

	len = get_entity_len(hash);
	if (len) {
		err = ssl_forward_request_entity(len);
	} else {
		err = 0;
	}

	return err;
}

static void show_cert(SSL *ssl)
{
	X509 *cert;
	char *line;

	cert = SSL_get_peer_certificate(client_ssl);
	if (!cert) {
		fprintf(stderr, "数字证书信息:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		fprintf(stderr, "证书: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		fprintf(stderr, "颁发者: %s\n", line);
		free(line);
		line = NULL;
		X509_free(cert);
	} else {
		fprintf(stderr, "无证书信息\n");
	}
}


static int ssl_associate_server(int connect_fd)
{
	int err = 0;

	/*兼容SSLv2和SSLv3*/
	server_ctx = SSL_CTX_new(SSLv23_server_method());
	if (!server_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载CA证书，第二参数为证书名，第三参数为证书地址*/
	err = SSL_CTX_load_verify_locations(server_ctx, CAFILE, NULL);
fprintf(stderr, "SSLerr1: %d\n", err);
	if (0 >=err) {
		fprintf(stderr, "ssl locations error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	SSL_CTX_set_default_passwd_cb_userdata(server_ctx, "123456");
fprintf(stderr, "SSLerr2: %d\n", err);
	if (0 >=err) {
		fprintf(stderr, "ssl passwd error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载公钥证书*/
	err = SSL_CTX_use_certificate_file(server_ctx, CERTF, SSL_FILETYPE_PEM);
fprintf(stderr, "SSLerr3: %d\n", err);
	if (0 >= err) {
		fprintf(stderr, "ssl server cert error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载私钥*/
	err = SSL_CTX_use_PrivateKey_file(server_ctx, KEYF, SSL_FILETYPE_PEM);
fprintf(stderr, "SSLerr4: %d\n", err);
	if (0 >= err) {
		fprintf(stderr, "ssl key error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*检查公钥与私钥是否匹配*/
	err = SSL_CTX_check_private_key(server_ctx);
fprintf(stderr, "SSLerr5: %d\n", err);
	if (0 >= err) {
		fprintf(stderr,"Private key does not match the certificate public key\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*SSL_VERIFY_NONE是的服务器不验证客户端的证书*/
	SSL_CTX_set_verify(server_ctx, SSL_VERIFY_NONE, NULL);
	/*建立ssl*/
	server_ssl = SSL_new(server_ctx);
fprintf(stderr, "SSLerr6: %d\n", err);
	if (server_ssl == NULL) {
		fprintf(stderr, "SSL_new error:");
		ERR_print_errors_fp(stderr);
		return -1;
	
	}
	/*与套接字关联*/
	err = SSL_set_fd(server_ssl,connect_fd);
fprintf(stderr, "SSLerr7: %d\n", err);
	if (err <= 0) {
		fprintf(stderr, "SSL_set_fd error:");
		ERR_print_errors_fp(stderr);
		return -1;
	
	}
	
	char reply[50];
	sprintf(reply, "%s", "HTTP/1.1 200 Connection Established\r\n\r\n");
	err = send(connect_fd, reply, 50, 0);
	if (err <= 0) {
		fprintf(stderr, "send error: %m\n");
		return -1;
	}

	int a = 1;
	while (a == 1) {
		err = SSL_accept(server_ssl);
fprintf(stderr, "SSLerr8: %d\n", err);
		if (err <= 0 && errno != EAGAIN) {
			fprintf(stderr, "SSL_ACCEPT error:");
			ERR_print_errors_fp(stdout);
			return -1;
		} else if (err < 0 && errno == EAGAIN) {
			perror("accept");
			a = 1;
		} else {
			a = 0;
		}
	}
	return 0;
}

int ssl_process(int connect_fd, start *line1, table *hash)
{

	int err = 0;
	/*初始化ssl库*/
	SSL_library_init();
	/*载入所有ssl错误信息*/
	SSL_load_error_strings();
	
	/*客户与代理进行SSL连接*/
	fprintf(stderr, "SSL_SERVER\n");
	err = ssl_associate_server(connect_fd);
	if (err == -1) {
		return -1;
	}

	return 0;
}


	/*目标服务器与代理进行SSL连接*/
	fprintf(stderr, "SSL_CLIENT\n");
	err = ssl_associate_client(client_socket);
	err = ssl_write_request_head(line1, hash);
	err = ssl_write_request_entity(hash);
	err = ssl_forward_response_message();
end:
	if (server_ctx)
	return err;
	

static int ssl_associate_client()
{
	int err = 0;

	SSL_set_fd(client_ssl, client_socket);
	
	err = SSL_connect(client_ssl);
	if (err == -1) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	printf("Connected with %s encryption\n", SSL_get_cipher(client_ssl));
	show_cert(client_ssl);

	return 0;
}

int https_client(char *ip, char *port, int connect_fd, start *line1, table *hash)
{

	int client_socket, addrlen, listen_socket;
	int err = 0;
        struct sockaddr_in addr;
	int port1;

	client_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!client_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	client_ssl = SSL_new(client_ctx);
	if (!client_ssl) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	/*完成服务器与客户进行ssl握手*/
	err = ssl_process(connect_fd, line1, hash);
	if (err) 
		return -1;
	/*port为char， 转为int型*/
	port1 = atoi(port);
	client_socket = socket(AF_INET, SOCK_STREAM, 0);//创建和服务器连接套接字  
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

        listen_socket = connect(client_socket, (struct sockaddr *)&addr, addrlen);
        if(listen_socket == -1) {
                fprintf(stderr,"Connect %s:%d error: %m\n", ip, port1);
		goto end;
	}

	struct timeval timeout = {1,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, 
		(char *)&timeout, sizeof(struct timeval));
	
	err = send_request_head(line1, hash);
	if (err) {
		goto end;
	}

	recv();
	
end:
	close(client_socket); 	
	return err;

}
