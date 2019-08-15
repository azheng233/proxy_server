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

/*代理做好接收浏览器SSL连接的准备：设定协议与加载证书*/
static int import_cert(int connect_fd)
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

	return 0;
}

/*接受客户SSL连接*/
static int accept_ssl_connect()
{
	int err;
	char reply[50];

	/*与套接字关联*/
	err = SSL_set_fd(server_ssl,connect_fd);
	if (err <= 0) {
		fprintf(stderr, "SSL_set_fd error:");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	sprintf(reply, "%s", "HTTP/1.1 200 Connection Established\r\n\r\n");
	err = send(connect_fd, reply, 50, 0);
	if (err <= 0) {
		fprintf(stderr, "send error: %m\n");
		return -1;
	}

	int a = 1;
	while (a == 1) {
		err = SSL_accept(server_ssl);
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

/*拒绝客户SSL连接*/
static int refuse_ssl_connect()
{
	int err;
	char reply[50];
	sprintf(reply, "%s", "HTTP 407 Unauthorized\r\n\r\n");
	err = send(connect_fd, reply, 50, 0);
	if (err <= 0) {
		fprintf(stderr, "send error: %m\n");
		return -1;
	}

	return 0;
}

/*关联代理与目标服务器*/
static int ssl_associate_client()
{
	int err = 0;

	client_ssl = SSL_new(client_ctx);
	if (!client_ssl) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

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



/*请求与目标服务器SSL连接*/
static int request_ssl_connect(start *line1, table *hash)
{
	int err;
	
	err = send_request_head(line1, hash);
	if (err) {
		return -1;
	}
	
	err = recv(client_socket, buf, 100, 0);
	if (err) {
		return -1;
	}
	buf[err - 4] = '\0';
	fprintf(stderr, "%s\n", buf);
	if (strstr(buf, "200") == NULL) {
		return -1;
	}

	return 0;
}
/*展示目标服务器证书*/
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

/*代理TCP连接目标服务器*/
int ssl_client(char *ip, char *port, int connect_fd, start *line1, table *hash)
{

	int addrlen, listen_socket;
	int err = 0;
        struct sockaddr_in addr;
	int port1;

	client_ctx = SSL_CTX_new(SSLv23_client_method());
	if (!client_ctx) {
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
	
	
end:
	close(client_socket); 	
	return err;
}

static int ssl_process()
{
	
}
