all: libpp64.so c64 server.prg rrserver.bin

libpp64.so: pp64.c pp64.h
	gcc -Wall -O3 -shared -fPIC -o libpp64.so pp64.c

c64: libpp64.so client.c
	gcc -L. -lpp64 -Wall -O3 -o c64 client.c	

server.prg: server.asm
	kasm -showmem -time -o server.prg server.asm

rrserver.bin: rrserver.asm
	kasm -showmem -time -binfile -o rrserver.bin rrserver.asm

install: libpp64.so c64
	cp pp64.h /usr/include
	cp libpp64.so /usr/lib
	cp c64 /usr/bin

uninstall:
	[ -f /usr/bin/c64 ] && rm -v /usr/bin/c64 || true
	[ -f /usr/lib/libpp64.so ] && rm -v /usr/lib/libpp64.so || true
	[ -f /usr/include/pp64.h ] && rm -v /usr/include/pp64.h || true

clean: 
	[ -f libpp64.so ] && rm -v libpp64.so || true
	[ -f c64 ] && rm -v c64 || true
	[ -f server.prg ] && rm -v server.prg || true
	[ -f rrserver.bin ] && rm -v rrserver.bin || true
