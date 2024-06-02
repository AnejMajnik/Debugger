CC = gcc
CFLAGS = -Wall -Wextra -Werror -m32

all: build run

build: debugger.c misc.c debuggee.c
	$(CC) $(CFLAGS) debugger.c misc.c -o npo_debug
	$(CC) $(CFLAGS) debuggee.c misc.c -o debuggee

run: npo_debug
	./npo_debug

clean:
	rm -f npo_debug debuggee

