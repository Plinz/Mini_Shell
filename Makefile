CC=gcc
CFLAGS=-Wall -g -std=c99 -Werror

all: miniShell

miniShell: miniShell.o readcmd.o

clean:
	rm -f shell shell.o readcmd.o miniShell miniShell.o
