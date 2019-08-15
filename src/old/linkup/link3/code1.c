#include<stdio.h> 
#include<stdlib.h>
#include <string.h>
 
      
void connect(char *st1, char *st2, char *q)  
{  
      
       	while(*st1!='\0') {  
            	*q=*st1;  
            	st1++;  
            	q++;  
        }     
        while (*st2!='\0') {  
            	*q=*st2;  
            	st2++;  
            	q++;  
        }  
        *q='\0';  
}  
      
int main()  
{  
        char buff_one[20], buff_new[20];  
        char *p;  
        char c[60]; 
	scanf("%s", buff_one); 
	setbuf(stdin, NULL);

	while(0 != strcmp(buff_one, "0")) {
		p = c;
	
		scanf("%s", buff_one); 
		setbuf(stdin, NULL);
		 
        	connect(buff_new,buff_one,p);
	
		strcpy(buff_new, p);  
	}	
	printf("THe connected string is :%s\n",p);  
    	return 0;  
}  
