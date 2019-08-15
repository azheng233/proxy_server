list: whitelist.o recvline.o
	gcc -o list whitelist.o recvline.o
whitelist: whitelist.c https.h recvline.h
	gcc -c whitelist.c
recvline.o: recvline.c https.h recvline.h
	gcc -c recvline.c
