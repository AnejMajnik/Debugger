CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: build run

build: main.c
	$(CC) $(CFLAGS) main.c -o npo_debug

run: npo_debug
	./npo_debug

clean:
	rm -f npo_debug main
