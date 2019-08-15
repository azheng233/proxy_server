#include "http.h"
#include "ssl_forward.h"
#include "sort_request.h"
#include "recvline.h"
#include "client.h"
#include "ssl_handle.h"

#define CAFILE "cert/root_cert.pem"
#define CERTF "cert/server_cert.pem"
#define	KEYF "cert/key.pem"

static SSL_CTX *server_ctx;
static SSL_CTX *client_ctx;
static SSL *server_ssl;
static SSL *client_ssl;
static int client_socket;


void refuse_ssl_connect(int connect_fd)
{
	char *reply;

	reply = malloc(50);
	sprintf(reply, "%s", "HTTP/1.1 407 Unauthorized\r\n\r\n");
	send(connect_fd, reply, 50, 0);
	
	free(reply);
	reply = NULL;
	return;
}

static int receive_request_result(int client_socket)
{
	int err;
	char *buf;
	buf = malloc(1000);
	err = recv(client_socket, buf, 999, 0);
	if (err == 0) {
		fprintf(stderr, "Target server disconnect\n");
		return -1;
	} else if (err == -1 && errno == EAGAIN) {
		fprintf(stderr, "receive_request_result timeout\n");
		return -1;
	} else if (err == -1 && errno != EAGAIN) {
		fprintf(stderr, "receive_request_result error: %m\n");
		return -1;
	}

	buf[err] = '\0';
	fprintf(stderr, "Request result: %s", buf);

	if (strstr(buf, "200") == NULL) {
		fprintf(stderr, "Target server refuse eonnect\n");
		free(buf);
		buf = NULL;
		return -1;
	}
	
	free(buf);
	buf = NULL;
	return 0;
}

static int ssl_connect_target(int client_socket)
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

	return 0;
}

int ssl_client(char *ip, char *port, int connect_fd, start *line1, table *hash)
{
	int addrlen;
	int port1;
	int err;
	struct sockaddr_in addr;
fprintf(stderr, "client_ctx\n");
	client_ctx = SSL_CTX_new(TLSv1_client_method());
	if (!client_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}
	
	port1 = atoi(port);
	fprintf(stderr, "%s %d\n", ip, port1);
    	client_socket = socket(AF_INET, SOCK_STREAM, 0);   //创建和服务器连接套接字  
    	if(client_socket == -1) {  
        	perror("socket");  
        	return -1;  
    	}	  

    	memset(&addr, 0, sizeof(addr));  
      
    	addr.sin_family = AF_INET;  /* Internet地址族 */  
    	addr.sin_port = htons(port1);  /* 端口号 */  
    	addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* IP地址 */  
    	inet_aton(ip, &(addr.sin_addr));  
  
	addrlen = sizeof(addr);  
	err =  connect(client_socket,  (struct sockaddr *)&addr, addrlen);     
	if(err == -1)  
   	{  
        	perror("connect");  
        	return -1;  
    	}  

	struct timeval timeout = {1,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, 
		(char *)&timeout, sizeof(struct timeval));

fprintf(stderr, "ssl_connect_target\n");
	err = ssl_connect_target(client_socket);
	if (err) {
		return -1;
	}
	
	return 0;
}

static void FREE_AND_CLOSE()
{
	if (client_ssl) {
		SSL_shutdown(client_ssl);
		SSL_free(client_ssl);
		SSL_CTX_free(client_ctx);
	} else if (client_ctx && !client_ssl) {
		SSL_CTX_free(client_ctx);
	}

	if (client_socket > 0)
		close(client_socket);

	if (server_ssl) {
		SSL_shutdown(server_ssl);
		SSL_free(server_ssl);
		SSL_CTX_free(server_ctx);
	} else if (server_ctx && !server_ssl) {
		SSL_CTX_free(server_ctx);
	}
	CONF_modules_free();
    	CONF_modules_unload(1);        //for conf  
        EVP_cleanup();                 //For EVP  
	ENGINE_cleanup();              //for engine  
	CRYPTO_cleanup_all_ex_data();  //generic   
	ERR_remove_state(0);           //for ERR  
	ERR_free_strings();            //for ERR  
	return;
}

static int import_cert()
{
	int err = 0;

	/*兼容SSLv2和SSLv3*/
	server_ctx = SSL_CTX_new(TLSv1_server_method());
	if (!server_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载CA证书，第二参数为证书名，第三参数为证书地址*/
	err = SSL_CTX_load_verify_locations(server_ctx, CAFILE, NULL);
	if (0 >=err) {
		fprintf(stderr, "ssl locations error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	SSL_CTX_set_default_passwd_cb_userdata(server_ctx, "123456");
	if (0 >=err) {
		fprintf(stderr, "ssl passwd error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载公钥证书*/
	err = SSL_CTX_use_certificate_file(server_ctx, CERTF, SSL_FILETYPE_PEM);
	if (0 >= err) {
		fprintf(stderr, "ssl server cert error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*加载私钥*/
	err = SSL_CTX_use_PrivateKey_file(server_ctx, KEYF, SSL_FILETYPE_PEM);
	if (0 >= err) {
		fprintf(stderr, "ssl key error\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*检查公钥与私钥是否匹配*/
	err = SSL_CTX_check_private_key(server_ctx);
	if (0 >= err) {
fprintf(stderr, "SSLerr1: %d\n", err);
		fprintf(stderr,"Private key does not match the certificate public key\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*SSL_VERIFY_NONE是的服务器不验证客户端的证书*/
	SSL_CTX_set_verify(server_ctx, SSL_VERIFY_NONE, NULL);

	return 0;
}

static int accept_ssl_request(int connect_fd)
{
	int err, n;
	char reply[50];

	/*建立ssl*/
	server_ssl = SSL_new(server_ctx);
	if (server_ssl == NULL) {
		fprintf(stderr, "SSL_new error:");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	/*与套接字关联*/
	err = SSL_set_fd(server_ssl, connect_fd);
	if (err <= 0) {
		fprintf(stderr, "SSL_set_fd error:");
		ERR_print_errors_fp(stderr);
		return -1;
	}

	n = sprintf(reply, "%s", "HTTP/1.1 200 Connection Established\r\n\r\n");
	err = send(connect_fd, reply, n, 0);
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

int ssl_process(char *ip, char *port, int connect_fd, start *line1, table *hash)
{
	int err = 0;
	start *ssl_line1;
	table *ssl_head;

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	err = import_cert();
	if (err) {
		refuse_ssl_connect(connect_fd);
		goto end;
	}

	err = accept_ssl_request(connect_fd);
	if (err) {
		goto end;
	}

	/*连接目标*/
	err = ssl_client(ip, port, connect_fd, line1, hash);
	if (err) {
		goto end;
	}

fprintf(stderr, "准备完成\n");
	err = SSL_forward(server_ssl, client_ssl);
	if (err) {
		fprintf(stderr, "代理失败\n");
		goto end;
	}
fprintf(stderr, "代理完成\n");
end:
	FREE_AND_CLOSE();
	return err;
}
