CC=gcc
CFLAGS=-Wall -W -g -std=c99 -Werror

miniShell: miniShell.o readcmd.o
	$(CC) -o miniShell miniShell.o readcmd.o $(CFLAGS)

miniShell.o : miniShell.c jobs.h readcmd.h
	$(CC) -o miniShell.o -c miniShell.c $(CFLAGS)

readcmd.o : readcmd.c readcmd.h
	$(CC) -o readcmd.o -c readcmd.c $(CFLAGS)

clean:
	rm -f shell shell.o readcmd.o miniShell miniShell.o 
