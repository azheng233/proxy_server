hansh: hanshu.o hanshu2.o
	gcc -o hansh hanshu.o hanshu2.o
hanshu.o: hanshu.c http.h hanshu2.h
	gcc -c hanshu.c
hanshu2.o: hanshu2.c http.h
	gcc -c hanshu2.c
