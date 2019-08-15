#include "ssl_handle.h"

static SSL *server_ssl;
static SSL *client_ssl;
static int client_socket;
static SSL_CTX *server_ctx;
static SSL_CTX *client_ctx;

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
	struct timeval timeout2 = {3,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, 
		(char *)&timeout2, sizeof(struct timeval));

	err =  connect(client_socket, (struct sockaddr *)&addr, addrlen);     
	if(err == -1) {  
        	perror("connect"); 
		client_ssl = NULL;
        	return -1;  
    	}  

	struct timeval timeout = {5,0};
	err = setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, 
		(char *)&timeout, sizeof(struct timeval));

	err = ssl_connect_target(client_socket);
	if (err) {
		return -1;
	}
	
	return 0;
}

static void FREE_AND_CLOSE()
{

	if (client_socket > 0)
		close(client_socket);

	if (server_ssl) {
		SSL_shutdown(server_ssl);
		SSL_free(server_ssl);
	}

	if (client_ssl) {
	//	SSL_shutdown(client_ssl);
		SSL_free(client_ssl);
	}
	return;
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

	err = SSL_accept(server_ssl);
	if (err <= 0 && errno != EAGAIN) {
		fprintf(stderr, "SSL_ACCEPT error:\n");
		return 1;
	} else if (err < 0 && errno == EAGAIN) {
		perror("accept");
		return 1;
	} 
		
	return 0;
}

int ssl_process(char *ip, char *port, int connect_fd, start *line1, table *hash, SSL_CTX *s_ctx, SSL_CTX *c_ctx, struct link *whitelist)
{

	int err = 0;
	start *ssl_line1;
	table *ssl_head;

 	server_ctx = s_ctx;
 	client_ctx = c_ctx;

	err = accept_ssl_request(connect_fd);
	if (err) {
		goto end;
	}

	/*连接目标*/
	err = ssl_client(ip, port, connect_fd, line1, hash);
	if (err) {
		goto end;
	}

	err = SSL_forward(server_ssl, client_ssl, line1, whitelist);
	if (err) {
		goto end;
	}
end:
	FREE_AND_CLOSE();
	fprintf(stderr, "over\n");

	return err;
}
