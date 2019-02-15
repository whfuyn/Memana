CC = gcc
INCLUDE = src/include
CFLAGS = -std=c11 -O2 -Wall
CFLAGS_O = $(CFLAGS) -c

all: first next best worst

clean:
	rm -f *.o first next best worst

first: basic first_fit.o
	$(CC) $(CFLAGS) test.o first_fit.o memana.o -o first

next: basic next_fit.o
	$(CC) $(CFLAGS) test.o next_fit.o memana.o -o next

best: basic best_fit.o
	$(CC) $(CFLAGS) test.o best_fit.o memana.o -o best

worst: basic worst_fit.o
	$(CC) $(CFLAGS) test.o worst_fit.o memana.o -o worst

basic: test.o memana.o

test.o: src/test.c 
	$(CC) $(CFLAGS_O) src/test.c -I $(INCLUDE) -o test.o

first_fit.o: src/first_fit.c $(INCLUDE)/memana.h
	$(CC) $(CFLAGS_O) src/first_fit.c -I $(INCLUDE) -o first_fit.o

next_fit.o: src/next_fit.c $(INCLUDE)/memana.h
	$(CC) $(CFLAGS_O) src/next_fit.c -I $(INCLUDE) -o next_fit.o

best_fit.o: src/best_fit.c $(INCLUDE)/memana.h
	$(CC) $(CFLAGS_O) src/best_fit.c -I $(INCLUDE) -o best_fit.o

worst_fit.o: src/worst_fit.c $(INCLUDE)/memana.h
	$(CC) $(CFLAGS_O) src/worst_fit.c -I $(INCLUDE) -o worst_fit.o

memana.o: src/memana.c $(INCLUDE)/memana.h
	$(CC) $(CFLAGS_O) src/memana.c -I $(INCLUDE) -o memana.o

