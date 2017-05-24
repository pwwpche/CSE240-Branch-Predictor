CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o predictor.o
	$(CC) $(OPTS) -lm -o predictor main.o predictor.o

main.o: main.c predictor.h
	$(CC) $(OPTS) -c main.c

predictor.o: predictor.h predictor.c
	$(CC) $(OPTS) -c predictor.c


clean:
	rm -f *.o predictor;
