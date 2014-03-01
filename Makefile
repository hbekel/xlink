GCC=gcc
GCC-MINGW32=i486-mingw32-gcc
FLAGS=-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -std=gnu99 -Wall -O3 -I.
KASM=java -jar /usr/share/kickassembler/KickAss.jar

all: linux win32 servers kernal

linux: c64
win32: c64.exe

libpp64.so: pp64.c pp64.h \
	driver/driver.c driver/driver.h \
	driver/usb.c driver/usb.h \
	driver/protocol.h \
	driver/parport.c driver/parport.h \
	extension.c extension.h extensions.c \
	util.c util.h 
	$(GCC) $(FLAGS) -shared -fPIC -Wl,-init,libpp64_initialize,-fini,libpp64_finalize -o libpp64.so pp64.c driver/driver.c driver/parport.c driver/usb.c extension.c util.c -lusb

c64: libpp64.so client.c client.h disk.c disk.h
	$(GCC) $(FLAGS) -o c64 client.c disk.c -L. -lpp64 -lreadline

pp64.dll: pp64.c pp64.h \
	driver/driver.c driver/driver.h \
	driver/usb.c driver/usb.h \
	driver/parport.c driver/parport.h \
	extension.c extension.h extensions.c \
	util.c util.h
	$(GCC-MINGW32) $(FLAGS) -shared -Wl,-init,libpp64_initialize,-fini,libpp64_finalize -o pp64.dll pp64.c driver/driver.c driver/parport.c driver/usb.c extension.c util.c -lusb

c64.exe: pp64.dll client.c client.h disk.c disk.h 
	$(GCC-MINGW32) $(FLAGS) -o c64.exe client.c disk.c -L. -lpp64

extensions.c: tools/compile-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | grep compile-extension | bash > extensions.c && rm extensions.bin

compile-extension: tools/compile-extension.c
	$(GCC) $(FLAGS) -o tools/compile-extension tools/compile-extension.c

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

install: c64
	install -m755 c64 /usr/bin
	install -m644 libpp64.so /usr/lib
	install -m644 pp64.h /usr/include
clean:
	[ -f libpp64.so ] && rm -v libpp64.so || true
	[ -f pp64.dll ] && rm -v pp64.dll || true
	[ -f c64 ] && rm -v c64 || true
	[ -f c64.exe ] && rm -v c64.exe || true
	[ -f extensions.c ] && rm -v extensions.c || true
	[ -f server.prg ] && rm -v server.prg || true
	[ -f rrserver.bin ] && rm -v rrserver.bin || true
	[ -f kernal-pp64.rom ] && rm -v kernal-pp64.rom || true
	[ -f tools/compile-extension ] && rm -v tools/compile-extension || true
	[ -f log ] && rm -v log || true

dist: zip clean
	(cd .. && tar vczf pp64.tar.gz pp64/)  

zip: all win32
	mkdir pp64-win32
	cp c64.exe pp64-win32
	cp pp64.dll pp64-win32
	cp pp64.h pp64-win32
	cp inpout32/inpout32.dll pp64-win32
	zip -r ../pp64-win32.zip pp64-win32
	rm -r pp64-win32
