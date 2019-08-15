proxy: main.o handle.o sort_request.o recvline.o client.o response_err.o
	gcc -o proxy main.o handle.o sort_request.o recvline.o client.o response_err.o
main.o: main.c http.h handle.h
	gcc -c main.c
handle.o: handle.c http.h sort_request.h  client.h response_err.h
	gcc -c handle.c
sort_request.o: sort_request.c http.h sort_request.h recvline.c
	gcc -c sort_request.c
recvline.o: recvline.c http.h recvline.h sort_request.c
	gcc -c recvline.c
client.o: client.c http.h client.h sort_request.h
	gcc -c client.c
response_err.o: response_err.c http.h response_err.c sort_request.h
	gcc -c response.c

clean:
	rm proxy main.o handle.o sort_request.o recvline.o client.o response_err.o
