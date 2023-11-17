CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99
LDFLAGS=-lX11 -lcurl -lXtst -lpthread

x11_keysender: main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
