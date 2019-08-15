server_link:main.o server.o
	gcc main.c server.c -o server_link
main.o:main.c server.h
	gcc -c main.c -o main.o
server.o:server.c server.h
	gcc -c server.c -o server.o
clean:
	rm -rf *.o server_link
