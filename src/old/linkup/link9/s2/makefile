server: main.o getlink.o gethash.o send.o
	gcc -o server main.o getlink.o gethash.o send.o
main.o: main.c http.h
	gcc -c main.c
getlink.o: getlink.c http.h
	gcc -c getlink.c
gethash.o: gethash.c http.h
	gcc -c gethash.c
send.o: send.c http.h
	gcc -c send.c
clear:
	rm server main.o getlink.o gethash.o send.o
