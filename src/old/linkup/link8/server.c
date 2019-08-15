#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8000
#define MAXLINE 100

static int nchildren;
static pid_t *pids;
static struct flock lock_it, unlock_it;
static int lock_fd = -1;

void conn(char *buff_new, char *buff_one, char *p)
{

        for(; *buff_new!='\0';) {
                *p=*buff_new;
                buff_new++;
                p++;
        }
        for(; *buff_one!='\0';) {

                *p=*buff_one;
                buff_one++;
                p++;
        }
        *p='\0';
}

struct link {
        char str[4096];
        struct link *next;
};

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        head->next = NULL;
	strcpy(head->str, "0");
        return head;
}

/*打印函数*/
void print(struct link *head)
{
        struct link *p;

        p = head->next;
        while (p != NULL) {
		if (strcmp(p->str, "\n"))
                	printf("%s ", p->str);
		else
			printf("Enter\n");

                p = p->next;
        }
	if (p = NULL)
		printf("aa\n");
}

int find_return (char *buf) 
{
	int i = 0;
	
	while (buf[i] != '\n') {
		if (buf[i] = '\0') {
			return 0;
		}
		
		i++;	
	}
}

static char *recv_link (int connect_fd) 
{
	static char *tail;
	static int tail_len;
	char *buf;
	int buflen = 0, len,return_pos;
	int size = 20;
	
	buf = malloc(size);

	if (buflen > size) {
		size *= 2;
		buf = (char *)realloc(buf, size);
	}

	if (tail) {
		buflen = tail_len;
		strncpy(buf, tail, tail_len);
	}
	
	len = recv(connect_fd, buf + buflen, size, 0);

	if (len >= 0) {
		buflen += len;
		if (buflen == strlen(buf)) {
			buf = (char *)realloc(buf, buflen+1);
		}
		
		buf[buflen] = '\0';
	} else {
		buf[0] = '\0';
		return buf;	
	}

	if (len == 0 && tail_len == 0) {
		buf[0] = '\0';
		free(tail);
		tail = NULL;
		return buf;
	}

	return_pos = find_return(buf);

	if (return_pos > 0) {
		tail_len = buflen - return_pos;

		if (tail_len) {
			tail = malloc(tail_len + 1);
			strncpy(tail, buf + return_pos, tail_len);
			tail[tail_len] = '\0';
		}
		
		buf[return_pos] = '\0';
		return buf;
	} 
}

struct link *resolve(char *buf) 
{
	int i = 0;	
	char bufi[2], node[1024];
	char *q;
	char a[1024];
	struct link *data, *p1, *p2;

	data = create();
	p1 = create();
	if (data == NULL || p1 == NULL) {
		printf("create error %m\n");
		exit(0);
	}
	data->next = p1;

	while (buf[i] == ' ' && buf[i] == '\n') {
		strcpy(node, &buf[i]);
		++i;
					
		while(1) {
			if (buf[i] == ' ') {
				++i;
				break;
			} else if (buf[i] == '\n')  {
				break;
			} else {
				strcpy(bufi, &buf[i]);
				++i;
				break;
			}
			q = a;
			conn(node, bufi, q);
			strcpy(node, q);
		}
		
		strcpy(p1->str, node);
		p2 = p1;

		if (buf[i] != '\n') {
			p1 = create();
		}
	}		
	
	return data;
}

struct link *receive(int connect_fd)
{
		
	struct link *data;
	char *buf;
	
	buf = recv_link(connect_fd);
	if (buf[0] == '\0') 
		data = create();
	else
		data = resolve(buf);
	return data;
}

int link_length(struct link *head)
{
        int length = 0;
        struct link *tmp;

        tmp = head->next;

        while (tmp != NULL) {
                length++;
                tmp = tmp->next;
        }

        return length;
}
/*相应客户端*/
void handle(int connect_fd)
{
	struct link *data, *linklist, *instruct;
	int n, length, i;

	while(1) {
		data = receive(connect_fd);
		if (data->next == NULL) 
			break;

		instruct = data->next;
		data = instruct->next;

		n = strcmp(instruct->str, "create");
		if (n = 0) {
			linklist = data;
			printf("received data:");
			print(linklist);

			length = link_length(linklist);
			printf("\nA total of %d data\n", length);
		}

		if (!linklist) {
			printf("Data is empty, %s fail\n");
			continue;
		}

		n = strcmp(instruct->str, "insert");
		n = strcmp(instruct->str, "create");
		n = strcmp(instruct->str, "create");
	}
	end: printf("disconnection\n");
}

void sig_chld(int signo)
{
	pid_t pid;
	int stat;
	printf("sig_chld\n");	

	pid == waitpid(-1, &stat, WNOHANG);
	
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
	printf("my_lock_wait\n");
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
	printf("my_lock_release\n");
	if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0) {
		printf("my_lock_release error: %s(errno: %d)\n", strerror(errno), errno);
	}
}

/*预先生成子进程*/
void child_main(int i, int socket_fd, int addrlen)
{
	int connect_fd;
	socklen_t clilen;
	struct sockaddr *cliaddr;
	char buff[4096];
	
	printf("执行child_main %d\n", i);	
	
	cliaddr = malloc(addrlen);
	
	printf("child %ld starting\n", (long)getpid());
	while(1) {
		clilen = addrlen;
		
		printf("======waiting for client's request======\n");
		
		my_lock_wait();
		connect_fd = (accept(socket_fd, cliaddr, &clilen));
		if (connect_fd < 0) {
			printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
			exit(0);
		} 
		my_lock_release();
		printf("welcome\n");
		handle(connect_fd);
		
	}
}

pid_t child_make(int i, int socket_fd, int addrlen)
{
	pid_t pid;
	
	printf("执行child_make %d\n", i);
	if((pid = fork()) > 0) {
		return (pid);
	}
	printf("调用child_main %d\n", i);
	child_main(i, socket_fd, addrlen);
}

/*信号处理*/
void sig_int(int signo)
{
	int i;
	printf("sig_int\n");	

	for (i = 0; i < nchildren; i++) {
		kill(pids[i], SIGTERM);
		printf("sig_int %d\n", i);	
	}
	
	while (wait(NULL) > 0)
		;
	if (errno != ECHILD) {
		printf("sif_int error: %s(errno: %d)\n",strerror(errno),errno);
	}

	exit(0);
}

typedef	void sigfunc(int);
sigfunc *signal(int signo, sigfunc *func)
{
	struct sigaction act, oact;
	
	printf("sigal\n");	

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
	
//	signal(SIGCHLD, sig_chld);

	nchildren = atoi(argv[argc-1]);
	pids = calloc(nchildren, sizeof(pid_t));

	my_lock_init("/tmp/lock.XXXXXX");	
	for (i = 0; i < nchildren; i++) {
		printf("调用child_make %d\n", i);
		pids[i] = child_make(i, socket_fd, addrlen);
	}

	signal(SIGINT, sig_int);
	
	while(1) {
		pause();
	}
	
	close(socket_fd);

	return 0;
}

