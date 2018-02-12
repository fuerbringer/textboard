CFLAGS=-lm -lpthread -Wall -g -O0
SOURCES=src/server.c src/posts.c src/helpers.c src/handler.c src/database.c
.PHONY: server src/static_files.h $(SOURCES)

server: src/static_files.h $(SOURCES)
	cc $(CFLAGS) -o $@ $(SOURCES)

src/static_files.h:
	echo "#pragma once" > $@
	cd static; \
	for i in *; do \
		printf "#define `echo $$i|cut -d. -f1` \"%s\"\n" "`cat $$i|xargs`" >> ../$@; \
	done

clean:
	rm -f server

