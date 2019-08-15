#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 8000
#define MAXLINE 100

static int nchildren;
static pid_t *pids;

void conn(char *buff_one, char *buff_new, char *p)
{

        for(; *buff_one!='\0';) {
                *p=*buff_one;
                buff_one++;
                p++;
        }
        for(; *buff_new!='\0';) {
                *p=*buff_new;
                buff_new++;
                p++;
        }
        *p='\0';
}

struct link {
        char num[4096];
        struct link *next;
};

struct link *create(void)
{
        struct link *head;

        head = (struct link *)malloc(sizeof (struct link) );
        head->next = NULL;
	strcpy(head->num, "0");
        return head;
}

char *receive(int connect_fd, char *buff_new)
{
	char buff_one[4096], buff[4096];
	char *p;
	int n;	

	memset(buff_one, 0, sizeof(buff_one));

        n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
        if (n == -1) {
                printf("recv error1: %s\n",strerror(errno),errno);
		exit(0);
        } else if (n == 0) {
		strcpy(buff_one, "Client-side disconnection");
	}
	
	if (buff_one[0] == '\r' && buff_one[1] == '\n') {
		printf("节点不得以回车开头");
        	n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
        	if (n == -1) {
                	printf("recv error: %s\n",strerror(errno),errno);
        	} else if (n == 0) {
			strcpy(buff_one, "Client-side disconnection");
		}	
	}

        strcpy(buff_new, buff_one);

        while (buff_one[0] != '\r' && buff_one[1] != '\n') {
                p = buff;

                n = recv(connect_fd, buff_one, sizeof(buff_one), 0);
                if (n == -1) {
                	printf("recv error2: %s\n",strerror(errno),errno);
			exit(0);
                } else if (n == 0) {
			break;
		}
                if (buff_one[0] == '\r' && buff_one[1] == '\n') {
                        break;
                }
      	 	buff_one[n] = '\0';
//		printf("recv:%s\n", buff_one);

                conn(buff_new,buff_one,p);

                strcpy(buff_new, p);
        }
	return buff_new;
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

void print(struct link *head)
{
        struct link *p;

        p = head->next;

        while (p != NULL) {
                printf("%s ", p->num);

                p = p->next;
        }
}

void handle(int connect_fd, char *buff_new)
{
	while(1) {
		struct link *head, *p1, *p2;
		int length;

		head = create();
		p1 = create();
		if (NULL == head || NULL == p1) {
		        printf ("内存分配失败");
		        exit (1);
		}

		strcpy(p1->num,receive(connect_fd, buff_new));
		printf("p1->num:%s\n", p1->num);
		if (0 == strcmp(p1->num, "Client-side disconnection")) {
			goto end;		
		}

		while (strcmp(p1->num, "0" )) {
		        if (head->next == NULL)
		                head->next = p1;
		        else
		                p2->next = p1;

		        p2 = p1;

		        p1 = create();
		        if (NULL == p1) {
		                printf ("内存分配失败");
		                exit (1);
		        }
			
			strcpy(p1->num,receive(connect_fd, buff_new));
		        printf("p1->num:%s\n", p1->num);
			if (0 == strcmp(p1->num, "Client-side disconnection")) {
                        	goto end;
                	}
		}
		
		printf("结束\n");	

		p2->next = NULL;

		if (head) {
		        print(head);
		} else {
		        printf("单向链表生成失败\n");
		        exit(1);
		}

		length = link_length(head);
		printf("\n链表长度为：%d\n", length);

		int n;
		n = recv(connect_fd, buff_new, sizeof(buff_new),0);
		if(n == 0) {
			close(connect_fd);
			break;
		} else {
			printf("Restart the main\n");
		}
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

void child_main(int i, int socket_fd, int addrlen)
{
	int connect_fd;
	socklen_t clilen;
	struct sockaddr *cliaddr;
	char buff_new[4096];
	
	printf("执行child_main %d\n", i);	
	
	cliaddr = malloc(addrlen);
	
	printf("child %ld starting\n", (long)getpid());
	while(1) {
		clilen = addrlen;
		printf("======waiting for client's request======\n");
		connect_fd = (accept(socket_fd, cliaddr, &clilen));
		if (connect_fd < 0) {
			printf("accept socket error: %s(errno: %d)\n",strerror(errno),errno);
			exit(0);
		} else {
			printf("welcome\n");
			handle(connect_fd, buff_new);
		}
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
	
	for (i = 0; i < nchildren; i++) {
		printf("调用child_make %d\n", i);
		pids[i] = child_make(i, socket_fd, addrlen);
	}

	signal(SIGINT, sig_int);
	
	while(1) {
		pause();
	}
	
	return 0;
}

