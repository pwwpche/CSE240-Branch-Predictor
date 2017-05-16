CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o predictor.o
	$(CC) $(OPTS) -o predictor main.o predictor.o -lm

main.o: main.c predictor.h
	$(CC) $(OPTS) -c main.c

predictor.o: predictor.h predictor.c
	$(CC) $(OPTS) -c predictor.c

perceptron.o: perceptron.h
	$(CC) $(OPTS) -c perceptron.h

neural.o: neural.h
		$(CC) $(OPTS) -c neural.h

clean:
	rm -f *.o predictor;
