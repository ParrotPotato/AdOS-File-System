main: main.o disk.o sfs.o debug.o
	gcc -o main main.o disk.o sfs.o debug.o
main.o: main.c disk.h sfs.h
	gcc -c -g main.c
sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c
disk.o: disk.c disk.h
	gcc -c -g disk.c
debug.o: debug.c debug.h
	gcc -c -g debug.c 

clean:
	rm -f main.o main disk.o sfs.o debug.o
