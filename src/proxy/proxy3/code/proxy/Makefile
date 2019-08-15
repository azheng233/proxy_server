proxy: main.o handle.o sort_request.o recvline.o judge_request.o client.o response_err.o
	gcc -o proxy main.o handle.o sort_request.o recvline.o judge_request.o client.o response_err.o
main.o: main.c http.h handle.h
	gcc -c main.c
handle.o: handle.c http.h sort_request.h judge_request.h client.h response_err.h
	gcc -c handle.c
sort_request.o: sort_request.c http.h sort_request.h recvline.c
	gcc -c sort_request.c
recvline.o: recvline.c http.h recvline.h sort_request.h
	gcc -c recvline.c
judge_request.o: judge_request.c judge_request.h sort_request.h
	gcc -c judge_request.c
client.o: client.c http.h client.h sort_request.h
	gcc -c client.c
response_err.o: response_err.c http.h response_err.h sort_request.h
	gcc -c response_err.c

clean:
	rm proxy main.o handle.o sort_request.o recvline.o client.o response_err.o
