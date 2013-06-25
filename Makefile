GCC=gcc
GCC-MINGW32=i486-mingw32-gcc
FLAGS=-Wall -O3
KASM=java -jar /usr/share/kickassembler/KickAss.jar

all: c64 c64.exe servers kernal

libpp64.so: pp64.c pp64.h
	$(GCC) $(FLAGS) -shared -fPIC -o libpp64.so pp64.c

pp64.dll: pp64.c pp64.h
	$(GCC-MINGW32) $(FLAGS) -shared -o pp64.dll pp64.c 

c64: libpp64.so client.c client.h disk.c disk.h
	$(GCC) $(FLAGS) -o c64 client.c disk.c -L. -lpp64

c64.exe: client.c client.h disk.c disk.h pp64.dll
	$(GCC-MINGW32) $(FLAGS) -o c64.exe client.c disk.c -L. -lpp64 

servers: server.prg rrserver.bin

server.prg: server.asm
	$(KASM) -o server.prg server.asm

rrserver.bin: rrserver.asm
	$(KASM) -binfile -o rrserver.bin rrserver.asm

kernal: cbm/kernal-901227-03.rom kernal-pp64.rom

kernal-pp64.rom: kernal.asm
	(cp cbm/kernal-901227-03.rom kernal-pp64.rom && \
	$(KASM) -binfile -o kernal.bin kernal.asm | \
	grep PATCH | \
	sed -r 's| +PATCH ([0-9]+) ([0-9]+)|dd conv=notrunc if=kernal.bin of=kernal-pp64.rom bs=1 skip=\1 seek=\1 count=\2 \&> /dev/null|' \
	> patch.sh) && sh -x patch.sh && rm -v patch.sh kernal.bin

clean:
	[ -f libpp64.so ] && rm -v libpp64.so || true
	[ -f pp64.dll ] && rm -v pp64.dll || true
	[ -f c64 ] && rm -v c64 || true
	[ -f c64.exe ] && rm -v c64.exe || true
	[ -f server.prg ] && rm -v server.prg || true
	[ -f rrserver.bin ] && rm -v rrserver.bin || true
	[ -f kernal-pp64.rom ] && rm -v kernal-pp64.rom || true

dist: bindist clean
	(cd .. && tar vczf pp64.tar.gz pp64/)  

zip: all
	mkdir pp64-win32
	cp c64.exe pp64-win32
	cp pp64.dll pp64-win32
	cp inpout32/inpout32.dll pp64-win32
	zip -r ../pp64-win32.zip pp64-win32
	rm -r pp64-win32
