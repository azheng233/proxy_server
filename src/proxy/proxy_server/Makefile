OBJECT = main.o handle.o sort_request.o recvline.o
OBJECT += judge_request.o client.o ssl_handle.o
OBJECT += ssl_forward.o response_err.o

TARGET = proxy
SSLLIB = -L/usr/local/openssl/lib -lssl -lcrypto -ldl
CC = gcc -g
CFILE = -I/usr/local/openssl/include -I.

$(TARGET): $(OBJECT)
	$(CC) -o $@ $(OBJECT) $(SSLLIB)

%.o: %.c %.h
	$(CC) -c $(CFILE) $<

main.o: main.c https.h recvline.h

.PHONY: clean
clean:
	rm -rf $(OBJECT) $(TARGET)
