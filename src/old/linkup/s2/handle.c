#include "http.h"
#include "recvline.h"
#include "edit_message.h"
#include "response.h"

static int judge_message (start *line1, int connect_fd) 
{
	char *URL;
	FILE *fd;
	char *xyline1;
	char *file;
	int n, i, err, len;

	URL = malloc(strlen(line1->url));
	URL[strlen(line1->url)-1] = '\0';
	strncpy(URL, line1->url +1, strlen(line1->url) -1);	

	err = strcmp(line1->method, "GET");
	if (err) {
		free_pointer(URL);
		return 501;
	}	

	err = strcmp(line1->version, "HTTP/1.1");
	if (err) {
		free_pointer(URL);
		return 505;
	}
	
	fd = fopen(URL, "r");
	if (fd == NULL) {
		free_pointer(URL);
		return 404;
	}
	
	err = send_file(connect_fd, fd, URL);		
	if (err) {
		free_pointer(URL);
		return err
	}
	free(URL);
	URL = NULL;

	return 0;
}

void handle(int connect_fd) 
{
	start *line1;
	table *hash;
	int n, err = 0;
	char *main_boby;
	char s[2];
	struct link *data;

	s[0] = ' '; 
	s[1] = '\0';

	data = receive_line(connect_fd, s);
	if (data == NULL) {
		err = 400;
	} else if (data->str[0] == '\0') {
		err = -1;
	} else if (data->str[0] == '\1') { 
		err = 500;
	}

	if (err) goto send_err;

	line1 = get_startline(data, line1);
	if (line1 == NULL) {
		err = 400;
		goto send_err;
	}

	s[0] = ':'; 
	s[1] = '\0';
        hash = malloc(sizeof(first)*BUCKET);
        for (n = 0; n < 10; n++) {
                hash->index[n] = NULL;
        }

	while (1) {
		data = receive_line(connect_fd, s);
	        if (data == NULL) {
			err = 400;
			break;
		} else if (data->str[0] == '\0') {
			err = -1;
			break;
		} else if (data->str[0] == '\1') { 
			err = 500;
			break;
		} else if (data->str[0] == '\r') { 
			break;
		}

		hash = get_first(data, hash);
		if (hash == NULL) {
printf("2\n");
			err = 400;
			break;
		}
	}
		
	if (err)
		goto send_err;		
	else
		main_boby = getboby(hash, connect_fd);
	
	printf("==================\n");
	err = judge_message(line1 ,connect_fd);
	printf("==================\n");

	send_err:
		if (err) {
			send_error(connect_fd, err);
		}
	
}

