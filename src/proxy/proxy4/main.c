#include "https.h"
#include "recvline.h"

#define CAFILE "cert/root_cert.pem"
#define CERTF "cert/server_cert.pem"
#define	KEYF "cert/key.pem"
#define	WHITE "config/white_list.conf"

SSL *server_ssl;
static int nchildren;
static pid_t *pids;
static struct flock lock_it, unlock_it;
static int lock_fd = -1;
static int sym;
SSL_CTX *server_ctx;
SSL_CTX *client_ctx;

void my_lock_init(char *pathname)
{
	char lock_file[1024];

	strncpy(lock_file, pathname, sizeof(lock_file));
	lock_fd = mkstemp(lock_file);
	
	unlink(lock_file);

	lock_it.l_type = F_WRLCK;
	lock_it.l_whence = SEEK_SET;
	lock_it.l_start = 0;
	lock_it.l_len = 0;

	unlock_it.l_type = F_UNLCK;
	unlock_it.l_whence = SEEK_SET;
	unlock_it.l_start = 0;
	unlock_it.l_len = 0;
}

void my_lock_wait()
{
	int rc;
	while ((rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0) {
		if(errno = EINTR) {
			continue;
		} else {
			printf("my_lock_wait error: %s(errno: %d)\n", strerror(errno), errno);
		} 
	}
}

void my_lock_release() 
{
	if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0) {
		printf("my_lock_release error: %s(errno: %d)\n", strerror(errno), errno);
	}
}

/*预先生成子进程*/
void child_main(int i, int socket_fd, struct link *whitelist)
{
	int err, connect_fd;
	int n = 0;
	
	fprintf(stderr, "child %ld starting\n", (long)getpid());
	while(1) {
		fprintf(stderr, "======waiting for client's request======\n");
		
		my_lock_wait();
		connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
		if (connect_fd < 0 ) {
			printf("accept error: %s(errno: %d)\n", strerror(errno), errno);
			exit(0);
		} 

		my_lock_release();

		struct timeval timeout = {3,0};
		err = setsockopt(connect_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
		if (err) {
			fprintf(stderr, "timeout error :%m(error: %d)", errno);
			exit(0);
		}
		fprintf(stderr, "welcome\n");
		while (n == 0) {
			n = handle(connect_fd, server_ctx, client_ctx, whitelist);
			fprintf(stderr, "n = %d\n", n);
		}
		close(connect_fd);
		n = 0;
	}
}

pid_t child_make(int i, int socket_fd, struct link *whitelist)
{
	pid_t pid;
	
	if((pid = fork()) > 0) {
		return (pid);
	}
	free(pids);
	pids = NULL;
	child_main(i, socket_fd, whitelist);
}

/*信号处理*/
void sig_int(int signo)
{
	sym = 0;
}

typedef	void Sigfunc(int);
Sigfunc *Signal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;

	fprintf(stderr,"signal\n");	

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	} else {
#ifdef SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}
	
	if (sigaction(signo, &act, &oact) < 0) {
		return(SIG_ERR);
	}
	return(oact.sa_handler);
}

static int import_cert(SSL_CTX *server_ctx)
{
	int err = 0;

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
		fprintf(stderr,"Private key does not match the certificate public key\n");
		ERR_print_errors_fp(stderr);
		return -1;
	}
	/*SSL_VERIFY_NONE是的服务器不验证客户端的证书*/
	SSL_CTX_set_verify(server_ctx, SSL_VERIFY_NONE, NULL);

	return 0;
}

static int setup_openssl() 
{
	int err;

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	/*TLSv1*/
	server_ctx = SSL_CTX_new(TLSv1_server_method());
	if (!server_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	err = import_cert(server_ctx);
	if (err) {
		return -1;
	}
	client_ctx = SSL_CTX_new(TLSv1_client_method());
	if (!client_ctx) {
		ERR_print_errors_fp(stderr);
		return -1;
	}

	return 0;
}

struct link *load_whitelist()
{
	struct link *whitelist, *p1, *p2;
	struct stat failinfo;
	int err, len, return_pos;
	int i, j;
	FILE *fd;
	char *buf, *tail = NULL;
	i = j = 0;
	err = stat(WHITE, &failinfo);
	if (err) {
		printf("stat error: %m\n");
		return whitelist;
	}

	buf = malloc(failinfo.st_size + 1);
	fd = fopen(WHITE, "r");
	if (fd == NULL) {
		fprintf(stderr, "白名单加载失败");
		return whitelist;
	}

	len = fread(buf, 1, failinfo.st_size, fd);
	if (ferror(fd)) {
		fprintf(stderr, "fread error\n");
		return whitelist;
	}

	buf[len] = '\0';
	
	whitelist = create();
/*	while (1) {
		p1 = create();
		if (tail) {
			sprintf(buf, "%s", tail);
			free(tail);
			tail = NULL;
		}
		return_pos = find_word(buf, "\n");
		if (return_pos) {
			strncpy(p1->str, buf, return_pos);
			p1->str[return_pos] = '\0';

			if (whitelist->next == NULL) 
				whitelist->next = p1;
			else
				p2->next = p1;

			p2 = p1;
			n += return_pos;
		} else {
			break;
		}

		if (len == n) 
			break;
		tail = malloc(len);
		sprintf(tail, "%s", buf + return_pos);
	}		
*/
	while (i < len) {
		p1 = create();
		
		while (buf[i] != '\n') {
			p1->str[j] = buf[i];
			++i;
			++j;
		}

		++i;
		p1->str[j + 1] = '\0';
		printf("url2:%s\n", p1->str);
		
		if (whitelist->next == NULL)
			whitelist->next = p1;
		else
			p2->next = p1;
		
		p2 = p1;
		j = 0;
	}

	p2->next = NULL;	
	print(whitelist);
	return whitelist;
}

int main (int argc, char **argv)
{
	int socket_fd, listen_fd, connect_fd;
	struct sockaddr_in seraddr;
        int on = 1;
	int i;
	socklen_t addrlen;
	struct link *whitelist;

	pid_t childpid;

	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("socket error: %s\n",strerror(errno),errno);
		exit(0);
	}

	memset(&seraddr, 0, sizeof(seraddr));

	seraddr.sin_family = AF_INET;
	seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	seraddr.sin_port = htons(PORT);
	
	if (-1 == (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))) {
		printf("setsockopt socket error: %s(errno: %d)\n",strerror(errno),errno);
		exit(0);
	}	

	addrlen = sizeof(seraddr); 	
	if (-1 == bind(socket_fd, (struct sockaddr *)&seraddr, addrlen)) {
		printf("bind socket error: %s\n",strerror(errno),errno);
		exit(0);
	}

	listen_fd = listen(socket_fd, 10);
	if (listen_fd == -1) {
		printf("listen socket error\n");
		exit(0);
	}

	signal(SIGPIPE, SIG_IGN);

	if (setup_openssl() == -1) {
		fprintf(stderr, "Setup openssl fail\n");
		exit(0);
	}
	
	whitelist = load_whitelist();
	if (whitelist->next == NULL) {
		destory(whitelist);
	}

	nchildren = atoi(argv[argc-1]);
	pids = calloc(nchildren, sizeof(pid_t));

	my_lock_init("/tmp/lock.XXXXXX");	
	for (i = 0; i < nchildren; i++) {
		pids[i] = child_make(i, socket_fd, whitelist);
	}
	
	sym = 1;
	signal(SIGINT, sig_int);

	while(sym) {
		pause();
	}

	SSL_CTX_free(server_ctx);
	SSL_CTX_free(client_ctx);
	CONF_modules_free();
    	CONF_modules_unload(1);        //for conf  
        EVP_cleanup();                 //For EVP  
	ENGINE_cleanup();              //for engine  
	CRYPTO_cleanup_all_ex_data();  //generic   
	ERR_remove_state(0);           //for ERR  
	ERR_free_strings();            //for ERR  

	for (i = 0; i < nchildren; i++) {
		kill(pids[i], SIGTERM);
		fprintf(stderr, "sig_int %d\n", i);         
	}   
	printf("free calloc: %p\n", pids);
	free(pids);
	pids = NULL;

	while (wait(NULL) > 0)
		;   
	if (errno != ECHILD) {
	        printf("sig_int error: %s(errno: %d)\n",strerror(errno),errno);                                 
        }

	close(socket_fd);

	return 0;
}

	
