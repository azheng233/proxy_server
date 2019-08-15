#include "http.h"
int main()
{
	int client_socket, addrlen, listen_socket, err, port1;
   	struct sockaddr_in addr;  

    	client_socket = socket(AF_INET, SOCK_STREAM, 0);   //创建和服务器连接套接字  

    	if(client_socket == -1) {  
        	perror("socket");  
        	return -1;  
    	}	  

    	memset(&addr, 0, sizeof(addr));  
      
    	addr.sin_family = AF_INET;  /* Internet地址族 */  
    	addr.sin_port = htons(PORT);  /* 端口号 */  
    	addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* IP地址 */  
    	inet_aton("119.75.217.109", &(addr.sin_addr));  
  
	addrlen = sizeof(addr);  
	listen_socket =  connect(client_socket,  (struct sockaddr *)&addr, addrlen);     
	if(listen_socket == -1)  
   	{  
        	perror("connect");  
        	return -1;  
    	}  
      
    	printf("成功连接到一个服务器\n");  
    
	printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
	
	return 0;
}
