ifeq ($(PRODUCTION), 1)
    CFLAGS=-lm -lpthread -Wall -O3
else
    CFLAGS=-lm -lpthread -Wall -g -O0
endif
SOURCES=src/server.c src/posts.c src/helpers.c src/handler.c src/database.c
.PHONY: server src/static_files.h $(SOURCES)

server: src/static_files.h $(SOURCES)
	cc $(CFLAGS) -o $@ $(SOURCES)

src/static_files.h:
	echo "#pragma once" > $@
	cd static; \
	for i in *; do \
		printf "#define `echo $$i|cut -d. -f1` " >> ../$@; \
		../enquote.py ./$$i >> ../$@; \
		printf "\n" >> ../$@; \
	done

clean:
	rm -f server

