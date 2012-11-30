all: libc64link.so c64 server.prg

libc64link.so: c64link.c c64link.h
	gcc -Wall -O2 -shared -fPIC -o libc64link.so c64link.c

c64: libc64link.so client.c
	gcc -L. -lc64link -o c64 client.c	

server.prg: server.asm
	kasm -showmem -time -o server.prg server.asm

install: libc64link.so
	cp c64link.h /usr/include
	cp libc64link.so /usr/lib
	cp c64 /usr/bin

uninstall:
	[ -f /usr/bin/c64 ] && rm -v /usr/bin/c64 || true
	[ -f /usr/lib/libc64link.so ] && rm -v /usr/lib/libc64link.so || true
	[ -f /usr/include/c64link.h ] && rm -v /usr/include/c64link.h || true

clean: 
	[ -f libc64link.so ] && rm -v libc64link.so || true
	[ -f c64 ] && rm -v c64 || true
	[ -f server.prg ] && rm -v server.prg || true
