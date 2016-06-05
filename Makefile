CFLAGS ?= -g -O2 -Wall -Wextra

PREFIX ?= /usr/local


all: scroll-daemon 

clean:
	$(RM) scroll-daemon
	$(RM) scroll.o

install: scroll-daemon
	install -m 0755 $^ $(PREFIX)/bin

uninstall:
	$(RM) $(PREFIX)/bin/scroll-daemon

scroll-daemon: scroll.o
	$(CC) -o $@ $^ $(LDFLAGS)

scroll.o: scroll.c
	$(CC) -c -o $@ $^ $(CFLAGS)

.PHONY: all clean install uninstall

