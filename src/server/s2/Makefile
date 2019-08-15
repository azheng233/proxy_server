server: main.o handle.o recvline.o edit_message.o response.o
	gcc -o server main.o handle.o recvline.o edit_message.o response.o
main.o: main.c http.h handle.h 
	gcc -c main.c
handle.o: handle.c http.h recvline.h edit_message.h response.h handle.h
	gcc -c handle.c
recvline.o: recvline.c http.h recvline.h edit_message.h
	gcc -c recvline.c
edit_message.o: edit_message.c http.h recvline.h edit_message.h
	gcc -c edit_message.c
response.o: response.c http.h recvline.h edit_message.h response.h
	gcc -c response.c

clean:
	rm server main.o handle.o recvline.o edit_message.o response.o
