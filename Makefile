CFLAGS=-lm -Wall -g -O0
.PHONY: server server.c posts.c helpers.c

server: server.c posts.c helpers.c
	cc $(CFLAGS) -o $@ $^

clean:
	rm -f server

