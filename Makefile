CFLAGS=-lm -lpthread -Wall -g -O0
SOURCES=server.c posts.c helpers.c handler.c database.c
.PHONY: server $(SOURCES)

server: server.c $(SOURCES)
	cc $(CFLAGS) -o $@ $^

clean:
	rm -f server

