OBJECT = main.o handle.o sort_request.o recvline.o \
	 judge_request.o client.o ssl_handle.o ssl_forward.o response_err.o
EXE = proxy
SSLLIB = -L/usr/local/openssl/lib -lssl -lcrypto -ldl
CC = gcc -g -I/usr/local/openssl/include

$(EXE): $(OBJECT)
	$(CC)-o $(EXE) $(OBJECT) $(SSLLIB)
%.o: %.c %.h https.h
	$(CC) -c %.c
main.o: main.c https.h
clean:
	rm $(EXE) $(OBJECT) 
