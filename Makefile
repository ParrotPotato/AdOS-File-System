all: main.o disk.o
	gcc main.o disk.o -o sfs


main.o: main.c
	gcc -c main.c -o main.o

disk.o: disk.c disk.h
	gcc -c disk.c -o disk.o
