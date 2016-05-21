all: list.o utilities.o
	mpicc -Wall -pthread list.o utilities.o main.c -o main `pkg-config --cflags --libs glib-2.0`

list.o: list.c
	gcc -c list.c `pkg-config --cflags --libs glib-2.0`

utilities.o: utilities.c
	gcc -c utilities.c

clean:
	rm -rf *o main

