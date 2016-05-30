all: list.o utilities.o
	mpicc -Wall -pthread list.o utilities.o main.c -o main 

list.o: list.c
	gcc -c list.c 

utilities.o: utilities.c
	gcc -c utilities.c

clean:
	rm -rf *o main

