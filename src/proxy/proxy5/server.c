#include "https.h"

static int nchildren;
static pid_t *pid;
static int sym;
static struct flock lock_it, unlock_it;
static int lock_fd = 1;

void child_main(int socket_fd, struct link *whitelist)
{
	int err, connect_fd;
	int n = 0;

	fprintf(stderr, "child %ld\n", (long)getpid);
	while(1) {
		my_lock_wait();
		connect_fd = accept(socket_fd, (struct sockaddr *)NULL, NULL);
		if (connect_fd < 0) {
			fprintf("accept error: %m(errno: %d)\n", errno);
			exit(0);
		}

		my_lock_release();

		struct timeval timeout = {3.0};
		err = setsockopt(connect, SOL_SOCKET, SO_RCVTIMEO, \
				(char *)&timeout, sizeof(struct timeval));
		if (err) {
			fprintf(stderr, "setsockopt(rcvtimeo) error: %m\n");
			exit(0);
		}

		while (!n) {
			n = handle(connect_fd, whitelist);
		}
		n = 0;
	}
}

pid_t child_make(int socket_fd, struct link *whitelist)
{
	pid_t pid;

	if ((pid = fork()) > 0)
		return (pid);
	free(pids);
	pids = NULL;
	child_main(socket_fd, whitelist);
}

void sig_int(int signo)
{
	sym = 0;
}

struct link *load_whitelist()
{
	struct link *whitelist, *p1, *p2;
	struct stat failinfo;
	int err, len, return_pos;
	int i = j = 0;
	FILE *fp;
	char *buf, tail = NULL;

	err = stat(WHITE, &failinfo);
	if (err) {
		fprintf("stat error: %m\n");
		exit(0);
	}

	fp = fopen(WHITE, "r");
	if (fp == NULL) {
		fprintf("fopen error: %m\n");
		exit(0);
	}

	buf = malloc(failinfo.st_size + 1);

	len = fread(buf, 1, failinfo.st_size, fd);
	if (ferror(fd)) {
		fprintf(stderr, "fread error\n");
		exit(0);
	}

	buf[len] = '\0';

	whitelist = create();
	while (i < len) {
		p1 = create();
		
		while (buf[i] == '\n') {
			p1->str[j] = buf[i];
			++i;
		}

		++i;
		p1->str[j + 1] = '\0';
		
		if (whitelist->next == NULL)
			whitelist->next = p1;
		else
			p2->next = p1;
		
		p2 = p1;
		j = 0;
	}

	p2->next = NULL;
	print(whitelist);
}
