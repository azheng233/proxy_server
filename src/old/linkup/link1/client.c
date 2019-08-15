#include <sys/types.h>  
#include <sys/socket.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <string.h>  
#include <arpa/inet.h>  
  
  
#define PORT 9990  
#define SIZE 1024  
  
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
    	inet_aton("192.168.56.101", &(addr.sin_addr));  
  
    	int addrlen = sizeof(addr);  
	int listen_socket =  connect(client_socket,  (struct sockaddr *)&addr, addrlen);     
	if(listen_socket == -1)  
   	{  
        	perror("connect");  
        	return -1;  
    	}  
      
    	printf("成功连接到一个服务器\n");  
      
    	char buf[SIZE] = {0};  
      
    	while(1) {  
        	printf("请输入你相输入的：");  
        	scanf("%s", buf);  
        	write(client_socket, buf, strlen(buf));  
          
        	int ret = read(client_socket, buf, strlen(buf));  
          
        	printf("buf = %s", buf);  
        	printf("\n");  
        	if(strncmp(buf, "END", 3) == 0) {  
            		break;  
        	}  
    	}  
    	close(listen_socket);  
      
    	return 0;  
}  
