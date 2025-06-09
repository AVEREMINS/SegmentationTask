CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lm

all: main run

main: main.c lodepng.c
	$(CC) $(CFLAGS) -o main main.c lodepng.c $(LIBS)

run: main
	./main
	
clean:
	rm -f main