link:main.o linklist.o
	gcc main.c linklist.c -o link
main.o:main.c linklist.h
	gcc -c main.c -o main.o
linklist.o:linklist.c linklist.h
	gcc -c linklist.c -o linklist.o
clean:
	rm -rf *.o link
