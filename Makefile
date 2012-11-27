all: libc64link.so c64link

libc64link.so: c64link.c c64link.h
	gcc -Wall -O2 -shared -fPIC -o libc64link.so c64link.c

c64link: libc64link.so client.c
	gcc -L. -lc64link -o c64link client.c	

install: libc64link.so
	cp c64link.h /usr/include
	cp libc64link.so /usr/lib
	cp c64link /usr/bin

uninstall:
	[ -f /usr/bin/c64link ] && rm -v /usr/bin/c64link || true
	[ -f /usr/lib/libc64link.so ] && rm -v /usr/lib/libc64link.so || true
	[ -f /usr/include/c64link.h ] && rm -v /usr/include/c64link.h || true

clean: 
	[ -f libc64link.so ] && rm -v libc64link.so || true
	[ -f c64link ] && rm -v c64link || true
