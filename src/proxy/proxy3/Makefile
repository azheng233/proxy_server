proxy: main.o handle.o sort_request.o recvline.o judge_request.o client.o ssl_handle.o ssl_forward.o response_err.o
	gcc -g -I/usr/local/openssl/include -o  proxy main.o handle.o sort_request.o recvline.o judge_request.o client.o ssl_handle.o ssl_forward.o response_err.o -L/usr/local/openssl/lib -lssl -lcrypto -ldl
main.o: main.c https.h handle.h
	gcc -c -g  main.c
handle.o: handle.c https.h sort_request.h judge_request.h client.h ssl_handle.h response_err.h
	gcc -c -g  handle.c
sort_request.o: sort_request.c https.h sort_request.h recvline.h
	gcc -c -g  -I/usr/local/openssl/include sort_request.c
recvline.o: recvline.c https.h recvline.h sort_request.h ssl_handle.h ssl_forward.h
	gcc -c -g  -I/usr/local/openssl/include recvline.c
judge_request.o: judge_request.c judge_request.h sort_request.h
	gcc -c -g  judge_request.c
client.o: client.c https.h sort_request.h recvline.h client.h
	gcc -c -g  client.c
ssl_handle.o: ssl_handle.c ssl_handle.h https.h sort_request.h recvline.h client.h ssl_forward.h
	gcc -c -g -I/usr/local/openssl/include ssl_handle.c
ssl_forward.o: ssl_forward.c ssl_forward.h https.h sort_request.h recvline.h client.h ssl_handle.h 
	gcc -c -g -I/usr/local/openssl/include ssl_forward.c
response_err.o: response_err.c https.h response_err.h sort_request.h
	gcc -c -g  response_err.c

clean:
	rm proxy main.o handle.o sort_request.o recvline.o client.o ssl_handle.o ssl_forward.o response_err.o judge_request.o
