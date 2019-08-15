#include <sys/types.h>  
#include <sys/socket.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <string.h>  
#include <arpa/inet.h>  
  
  
#define PORT 80 
#define SIZE 4096  
  
int main()  
{  
    	int client_socket = socket(AF_INET, SOCK_STREAM, 0);   //创建和服务器连接套接字  

    	if(client_socket == -1) {  
        	perror("socket");  
        	return -1;  
    	}	  

   	struct sockaddr_in addr;  
    	memset(&addr, 0, sizeof(addr));  
      
    	addr.sin_family = AF_INET;  /* Internet地址族 */  
    	addr.sin_port = htons(PORT);  /* 端口号 */  
    	addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* IP地址 */  
    	inet_aton("119.75.217.109", &(addr.sin_addr));  
  
	int addrlen = sizeof(addr);  
	int listen_socket =  connect(client_socket,  (struct sockaddr *)&addr, addrlen);     
	if(listen_socket == -1)  
   	{  
        	perror("connect");  
        	return -1;  
    	}  
      
    	printf("成功连接到一个服务器\n");  
    
//	FILE *stream;
        char buff_one[4096];
//      int i = 0;
/*
        if ((stream = fopen("text.txt", "r")) == NULL) {
                fprintf(stderr,"Can not open output file.\n");
                return 0;
        }
               // fseek(stream, i, SEEK_SET);
        fread(buff_one, sizeof(buff_one), 1, stream);
        
        while (buff_one[i] != '\0') {
                i++;
        }
	printf("%d\n", i-1);	
        printf("%s", buff_one);
*/
	sprintf(buff_one, "GET /index.html HTTP/1.1\r\nHost:www.baidu.com\r\n\r\n");
	if ((send(client_socket, buff_one, 100, 0)) < 0) {
		printf("error\n");
	}
	
//        fclose(stream);
	recv(client_socket, buff_one, SIZE, 0);
	printf("%s\n", buff_one);
    	close(listen_socket);  
      
    	return 0;  
}  
