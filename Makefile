GCC=gcc
#GCC-MINGW32=i486-mingw32-gcc
GCC-MINGW32=i686-pc-mingw32-gcc
FLAGS=-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -std=gnu99 -Wall -O3 -I.
#KASM=java -jar /usr/share/kickassembler/KickAss.jar
KASM=java -jar c:/cygwin/usr/share/kickassembler/KickAss.jar

all: linux win32 servers kernal

linux: xlink
win32: xlink.exe

libxlink.so: xlink.c xlink.h \
	driver/driver.c driver/driver.h \
	driver/usb.c driver/usb.h \
	driver/protocol.h \
	driver/parport.c driver/parport.h \
	extension.c extension.h extensions.c \
	util.c util.h 
	$(GCC) $(FLAGS) -shared -fPIC -Wl,-init,libxlink_initialize,-fini,libxlink_finalize -o libxlink.so xlink.c driver/driver.c driver/parport.c driver/usb.c extension.c util.c -lusb

xlink: libxlink.so client.c client.h disk.c disk.h
	$(GCC) $(FLAGS) -o xlink client.c disk.c -L. -lxlink -lreadline

xlink.dll: xlink.c xlink.h \
	driver/driver.c driver/driver.h \
	driver/usb.c driver/usb.h \
	driver/parport.c driver/parport.h \
	extension.c extension.h extensions.c \
	util.c util.h
	$(GCC-MINGW32) $(FLAGS) -shared -o xlink.dll xlink.c driver/driver.c driver/parport.c driver/usb.c extension.c util.c -lusb

xlink.exe: xlink.dll client.c client.h disk.c disk.h 
	$(GCC-MINGW32) $(FLAGS) -o xlink.exe client.c disk.c -L. -lxlink

extensions.c: tools/compile-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | grep compile-extension | bash > extensions.c && rm extensions.bin

compile-extension: tools/compile-extension.c
	$(GCC) $(FLAGS) -o tools/compile-extension tools/compile-extension.c

servers: server.prg rrserver.bin

server.prg: server.asm
	$(KASM) -o server.prg server.asm

rrserver.bin: rrserver.asm
	$(KASM) -binfile -o rrserver.bin rrserver.asm

kernal: cbm/kernal-901227-03.rom kernal-xlink.rom

kernal-xlink.rom: kernal.asm
	(cp cbm/kernal-901227-03.rom kernal-xlink.rom && \
	$(KASM) -binfile -o kernal.bin kernal.asm | \
	grep PATCH | \
	sed -r 's| +PATCH ([0-9]+) ([0-9]+)|dd conv=notrunc if=kernal.bin of=kernal-xlink.rom bs=1 skip=\1 seek=\1 count=\2 \&> /dev/null|' \
	> patch.sh) && sh -x patch.sh && rm -v patch.sh kernal.bin

install: xlink
	install -m755 xlink /usr/bin
	install -m644 libxlink.so /usr/lib
	install -m644 xlink.h /usr/include
	[ -d /etc/bash_completion.d ] && install -m644 etc/bash_completion.d/xlink /etc/bash_completion.d/

uninstall:
	rm -v /usr/bin/xlink
	rm -v /usr/lib/libxlink.so
	rm -v /usr/include/xlink.h
	[ -f /etc/bash_completion.d/xlink ] && rm -v /etc/bash_completion.d/xlink

clean:
	[ -f libxlink.so ] && rm -v libxlink.so || true
	[ -f xlink.dll ] && rm -v xlink.dll || true
	[ -f xlink ] && rm -v xlink || true
	[ -f xlink.exe ] && rm -v xlink.exe || true
	[ -f extensions.c ] && rm -v extensions.c || true
	[ -f server.prg ] && rm -v server.prg || true
	[ -f rrserver.bin ] && rm -v rrserver.bin || true
	[ -f kernal-xlink.rom ] && rm -v kernal-xlink.rom || true
	[ -f tools/compile-extension ] && rm -v tools/compile-extension || true
	[ -f log ] && rm -v log || true

dist: zip clean
	(cd .. && tar vczf xlink.tar.gz xlink/)  

zip: all win32
	mkdir xlink-win32
	cp xlink.exe xlink-win32
	cp xlink.dll xlink-win32
	cp xlink.h xlink-win32
	cp inpout32/inpout32.dll xlink-win32
	zip -r ../xlink-win32.zip xlink-win32
	rm -r xlink-win32
