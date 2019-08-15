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
#include <time.h> 

#define PORT 8000
#define MAXLINE 100
#define BUCKET 10

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

typedef struct request_start {
	char method[10];
	char url[100];
	char version[100];
}start;

typedef struct request_first {
	char *key;
	char *value;
	struct request_first *next;
}first;

typedef struct hash_table {
	first *index[10];
}table;
/*
int ELF_hash (char *command)
{
        unsigned  int hash = 5381;

        if (command == NULL) {
                return -1;
        }

        while (*command) {
                hash += (hash << 5) + (*command++);
        }
printf("hash: %d\n", (hash & 0x7FFFFFFF)%BUCKET);
        return (hash & 0x7FFFFFFF)%BUCKET; 

}
*/
int ELF_hash (char *cmd)
{
	unsigned int hash=0;  
	unsigned int x=0;  
	while (*cmd)  
	{  
	    	hash = (hash<<4) + *cmd;  
	    	if ((x=hash & 0xf0000000) != 0)  
	   	{  
	        	hash ^= (x>>24);    
	        	hash &= ~x;  
	    	}  
	    	cmd++;    
	}  
printf("hash:%d\n", (hash & 0x7fffffff)%BUCKET);

	return (hash & 0x7fffffff)%BUCKET; 
}


first *find_hash (table *hash, char *find_data) 
{
	first *q;
	int i, n;
	
	i = ELF_hash(find_data);
	q = hash->index[i];
	if (q == NULL) {
		return q;
	}
		
	while (1) {
		n = strcmp(q->key, find_data);
		if (n == 0) {
			return q;
		} else {
			if (q->next != NULL)
				q = q->next;
			else {
				q->key = NULL;
				q->value = NULL;
				q->next = NULL;	
				return q;
			}
		}
	}
}

void print_hash (table *hash) 
{
	first *q;
	int i;
	for (i = 0; i < BUCKET; i++) {
		printf("Index[%d]:", i);	
	
		q = hash->index[i];
		while (q != NULL) {

			printf("key(%s),value:(%s)->", q->key, q->value);
			q = q->next;
		}
		printf("NULL\n");
	}
}

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
                printf("%s ", p->str);
                p = p->next;
        }
	printf("\n");
}

int find_return (char *buf) 
{
	int i = 0;
	
	while (buf[i] != '\n') {
		if (buf[i] == '\0') {
			return 0;
		}
		
		i++;	
	}
	return i+1;
}

struct link *resolve(char *buf, char *symbol) 
{
	int i = 0;	
	char bufi[2], node[1024];
	char *q;
	char a[1024];
	struct link *data, *p1, *p2;
	data = create();
	p1 = create();
	if (data == NULL || p1 == NULL) {
		printf("create error\n");
		exit(0);
	}

	while (buf[i] != symbol[0] && buf[i] != '\r') {
		node[0] = buf[i];
		node[1] = '\0';
		++i;
					
		while(1) {
			if (buf[i] == symbol[0] && symbol[0] == ' ') {
				i++; 
			        break;  	
			}                              	
			if (buf[i] == ':' && buf[i+1] == ' ') {
				i += 2;
				break;
			} 
			if (buf[i] == '\r')
				break;
			bufi[0] = buf[i];
			bufi[1] = '\0'; 
			++i;
			q = a;
			conn(node, bufi, q);
			strcpy(node, q);
			node[i+1] = '\0';
		}

		if (data->next == NULL)
			data->next = p1;
		else 
			p2->next = p1;

		strcpy(p1->str, node);
		p2 = p1;
		if (buf[i] != '\n') {
			p1 = create();
		}
	}		
	p2->next = NULL;
	return data;
}

static char *recv_link (int connect_fd) 
{
	static char *tail;
	static int tail_len = 0;
	char *buf, *newsize;
	int buflen = 0, len,return_pos;
	int size = 20;
	
	buf = malloc(size);
	while (1) {
int a=0;
		if (buflen >= size) {
			size += 20;
			buf = (char *)realloc(buf, size);
		}
	
		if (tail) {
			buflen = tail_len;
			strncpy(buf, tail, tail_len);
		}

		len = recv(connect_fd, buf + buflen, size - buflen, MSG_DONTWAIT);
		if (len > 0) {
			buflen += len;
		
		} else if (len == 0){
                        free(tail);
                        tail = NULL;
                        tail_len = 0;
                        buf[0] = '\0';

                        return buf;			
		} else if (len < 0 && errno != EAGAIN) {
			printf("recv error:%m");
                        free(tail);
                        tail = NULL;
                        tail_len = 0;
			buf[0] = '\0';

			return buf;	
		} else {
			if (0 == buflen) {
				continue;
			}
		}
		
		buf = (char *)realloc(buf, buflen + 1);
		buf[buflen] = '\0';
	
		return_pos = find_return(buf);
	
		if (return_pos > 2) {
			tail_len = buflen - return_pos;
	
			if (tail_len) {
				tail = malloc(tail_len + 1);
				strncpy(tail, buf + return_pos, tail_len);
				tail[tail_len] = '\0';
			}
			
			buf[return_pos] = '\0';
printf("buf:%s", buf);
			return buf;
		} else if (return_pos == 2) {
			free(tail);
			tail = NULL;
			tail_len = 0;
			buf[1] = '\0';
			return buf;
		} else {
			tail = NULL;
		}
	}
}

struct link *receive(int connect_fd, char *symbol)
{
		
	struct link *data;
	char *buf;
	
	buf = recv_link(connect_fd);

	if (buf[0] == '\0') {
		data = NULL;
	} else if (buf[0] == '\r') {
		data = create();	
		strcpy(data->str, "\n");
	} else {
		data = resolve(buf, symbol);
	}

	return data;
}

int response (start *line1, int connect_fd) 
{
        FILE *fd;
        char buf[4096];
	char message[4096];
        int n, err, len, i = 0;
	char *URL;
	time_t t;
	struct tm *timeinfo;

	n = strcmp(line1->method, "GET");
	if (n) {
		printf("请求方式非GET\n");

		sprintf(buf, "HTTP/1.1 501 Not Implemented\n");
		send(connect_fd, buf, strlen(buf), 0);
		return 1;
	}

	time(&t);
	timeinfo = localtime(&t);	

	URL = malloc(strlen(line1->url));
	URL[strlen(line1->url)-1] = '\0';
	strncpy(URL, line1->url +1, strlen(line1->url) -1);	

        fd = fopen(URL, "r");
        if (fd == NULL) {
                return err = 1;
        }

	fread(buf, sizeof(buf), 1, fd);

	n = sprintf(message, "HTTP/1.1 200 OK\r\nServer: weizheng\r\nDate: ");
	len = n;
	n = sprintf(message + len, asctime(timeinfo));
	len += n; --len;	
	n = sprintf(message + len, "\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: ");
	len += n;
	n = sprintf(message + len, "%d", strlen(buf));
	len += n;
	sprintf(message + len, "\r\nConnection: close\r\n\r\n");

	send(connect_fd, message, strlen(message), 0);
	send(connect_fd, buf, strlen(buf), 0);
	printf("%s", message);
	printf("%s\n", buf);

	return 0;
}

/*相应客户端*/
start *get_startline (int connect_fd)
{
	struct link *data;
	int n, i, l;
	char symbol[2];
	start *line1;
	
	symbol[0] = ' ';
	symbol[1] = '\0';
	
printf("================================================\n");	
	data = receive(connect_fd, symbol);
	if (data == NULL)
		return line1 = NULL;

	print(data);
	printf("\n");
	
	line1 = malloc(sizeof(start));
	data = data->next;
	strcpy(line1->method, data->str);
	
	data = data->next;
	strcpy(line1->url, data->str);

	data = data->next;
	strcpy(line1->version, data->str);
	
	return line1;
}

table *get_first (int connect_fd) 
{
	struct link *data;
	int n, i;
	first *entry, *front;
	char symbol[2];
	table *hash;

	symbol[0] = ':';
	symbol[1] = '\0';

	hash = malloc(sizeof(first)*BUCKET);
	for (n = 0; n < 10; n++) {
		hash->index[n] = NULL;
	}

	while (1) {
		data = receive(connect_fd, symbol);
		if (data == NULL)
			return hash = NULL;

		if (0 == strcmp(data->str, "\n")) {
print_hash(hash);
printf("SHOUBUJIESHU\n");
			return hash;
		}
	
		if (data->next == NULL) {
			return hash;
		}

		entry = malloc(sizeof(first));	
		entry->next = NULL;

		data = data->next;
		entry->key = malloc(strlen(data->str));
		strcpy(entry->key, data->str);

		data = data->next;
		entry->value = malloc(strlen(data->str));
		strcpy(entry->value, data->str);
		i = ELF_hash(data->str);	
		if (hash->index[i] == NULL) {
			hash->index[i] = entry;
		} else {
			front = hash->index[i];

			while (front->next != NULL) {
				front = front->next;
			}
			front->next = entry;
		}
	}
}	

void handle (int connect_fd) 
{
	start *line1;
	table *hash;
	char *main_boby;
	first *accept_len;
	int len, n;

	line1 = get_startline(connect_fd);
	if (line1 == NULL) {
		return;
	}
	hash = get_first(connect_fd);
	if (hash == NULL) {
		return;
	}	
	accept_len = find_hash(hash, "Accept-Length");
printf("******************\n");
	if (accept_len) {
		len = atoi(accept_len->value);
		main_boby = malloc(len);
		recv(connect_fd, main_boby, len+1, 0);
		main_boby[len] = '\n';
	} else {
		printf("No main boby\n");
	}
	n = response(line1 ,connect_fd);
printf("==================\n");
	
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

