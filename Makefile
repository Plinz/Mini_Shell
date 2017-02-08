CC=gcc
CFLAGS=-Wall -g

all: miniShell shell

miniShell: miniShell.o readcmd.o

shell: shell.o readcmd.o

clean:
	rm -f shell shell.o readcmd.o miniShell miniShell.o
