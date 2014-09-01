GCC=gcc
#GCC-MINGW32=i486-mingw32-gcc
GCC-MINGW32=i686-pc-mingw32-gcc
FLAGS=-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -std=gnu99 -Wall -O3 -I.
#KASM=java -jar /usr/share/kickassembler/KickAss.jar
KASM=java -jar c:/cygwin/usr/share/kickassembler/KickAss.jar

LIBHEADERS=\
	xlink.h \
	util.h \
	extension.h \
	driver/driver.h \
	driver/protocol.h \
	driver/usb.h \
	driver/parport.h

LIBSOURCES=\
	xlink.c \
	util.c \
	extension.c \
	extensions.c \
	driver/driver.c \
	driver/usb.c \
	driver/parport.c

all: linux server kernal bootstrap
linux: xlink
win32: xlink.exe
server: xlink-server.prg
kernal: cbm/kernal-901227-03.rom xlink-kernal.rom

libxlink.so: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC) $(FLAGS) -shared -fPIC \
		-Wl,-init,libxlink_initialize,-fini,libxlink_finalize\
		-o libxlink.so $(LIBSOURCES) -lusb

xlink: libxlink.so client.c client.h disk.c disk.h
	$(GCC) $(FLAGS) -o xlink client.c disk.c -L. -lxlink -lreadline

xlink.dll: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC-MINGW32) $(FLAGS) -shared -o xlink.dll $(LIBSOURCES) -lusb

xlink.exe: xlink.dll client.c client.h disk.c disk.h 
	$(GCC-MINGW32) $(FLAGS) -o xlink.exe client.c disk.c -L. -lxlink

extensions.c: tools/compile-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | \
	grep compile-extension | \
	bash > extensions.c && rm extensions.bin

compile-extension: tools/compile-extension.c
	$(GCC) $(FLAGS) -o tools/compile-extension tools/compile-extension.c

compile-basicloader: tools/compile-basicloader.c
	$(GCC) $(FLAGS) -o tools/compile-basicloader tools/compile-basicloader.c

xlink-server.prg: server.asm
	$(KASM) -o xlink-server.prg server.asm

bootstrap: compile-basicloader bootstrap.asm
	$(KASM) -o bootstrap.prg bootstrap.asm && \
	tools/compile-basicloader bootstrap.prg > bootstrap.bas && \
	rm -v bootstrap.prg

xlink-kernal.rom: kernal.asm
	(cp cbm/kernal-901227-03.rom xlink-kernal.rom && \
	$(KASM) -binfile -o kernal.bin kernal.asm | \
	grep PATCH | \
	sed -r 's| +PATCH ([0-9]+) ([0-9]+)|dd conv=notrunc if=kernal.bin of=xlink-kernal.rom bs=1 skip=\1 seek=\1 count=\2 \&> /dev/null|' \
	> patch.sh) && \
	sh -x patch.sh && \
	rm -v patch.sh kernal.bin

firmware: driver/at90usb162/xlink.c driver/at90usb162/xlink.h
	(cd driver/at90usb162 && make)

firmware-clean:
	(cd driver/at90usb162 && make clean)

firmware-install: xlink firmware
	LD_LIBRARY_PATH=. ./xlink bootstrap && \
	(cd driver/at90usb162 && make dfu)

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

clean: firmware-clean
	[ -f libxlink.so ] && rm -v libxlink.so || true
	[ -f xlink.dll ] && rm -v xlink.dll || true
	[ -f xlink ] && rm -v xlink || true
	[ -f xlink.exe ] && rm -v xlink.exe || true
	[ -f extensions.c ] && rm -v extensions.c || true
	[ -f xlink-server.prg ] && rm -v xlink-server.prg || true
	[ -f xlink-kernal.rom ] && rm -v xlink-kernal.rom || true
	[ -f bootstrap.bas ] && rm -v bootstrap.bas || true
	[ -f tools/compile-extension ] && rm -v tools/compile-extension || true
	[ -f tools/compile-basicloader ] && rm -v tools/compile-basicloader || true
	[ -f log ] && rm -v log || true

dist: clean
	(cd .. && tar vczf xlink.tar.gz xlink/)

