#include "http.h"
#include "handle.h"

static int nchildren;
static pid_t *pids;
static struct flock lock_it, unlock_it;
static int lock_fd = -1;
static int sym;
void sig_chld(int signo)
{
	pid_t pid;
	int stat;
	printf("sig_chld\n");	

	pid == waitpid(-1, &stat, WNOHANG);
	
	free(pids);
	pids = NULL;

	return;
}

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
void child_main(int i, int socket_fd)
{
	int err, connect_fd;
	socklen_t clilen;
	struct sockaddr *cliaddr;
	int n = 2;
	
	fprintf(stderr, "child %ld starting\n", (long)getpid());
	while(1) {
		fprintf(stderr, "======waiting for client's request======\n");
		
		my_lock_wait();
		connect_fd = (accept(socket_fd, (struct sockaddr *)NULL, NULL));
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
		while (n == 2) {
			n = handle(connect_fd);
			fprintf(stderr, "n = %d\n", n);
		}
		close(connect_fd);
		n = 2;
	}
}

pid_t child_make(int i, int socket_fd)
{
	pid_t pid;
	
	if((pid = fork()) > 0) {
		return (pid);
	}
	free(pids);
	pids = NULL;
	child_main(i, socket_fd);
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
/*文件上锁保护*/

int main (int argc, char **argv)
{
	int socket_fd, listen_fd, connect_fd;
	struct sockaddr_in seraddr;
        int on = 1;
	int i;
	socklen_t addrlen;

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
	nchildren = atoi(argv[argc-1]);
	pids = calloc(nchildren, sizeof(pid_t));

	my_lock_init("/tmp/lock.XXXXXX");	
	for (i = 0; i < nchildren; i++) {
		pids[i] = child_make(i, socket_fd);
	}

	sym = 1;
	Signal(SIGINT, sig_int);
	
	while(sym) {
		pause();
	}
	for (i = 0; i < nchildren; i++) {
		kill(pids[i], SIGTERM);
		fprintf(stderr, "sig_int %d\n", i);         
	}   

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

	
