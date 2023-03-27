PREFIX=/usr
SYSCONFDIR=/etc
KASM?=kasm3

MINGW32?=i686-w64-mingw32
MINGW32-GCC=$(MINGW32)-gcc
MINGW32-WINDRES=$(MINGW32)-windres
MINGW32-CFLAGS=-DCLIENT_VERSION="$(VERSION)" -std=gnu99 -Wall -Wno-format-security \
	-O3 -I. -I/usr/$(MINGW32)/include

VERSION=1.3
XLINK_SERIAL:=$(XLINK_SERIAL)

SHELL=/bin/bash
CC?=gcc
GCC=gcc
CFLAGS=-DCLIENT_VERSION="$(VERSION)" -std=gnu99 -Wall -Wno-format-security -O3 \
	-I. -I$(PREFIX)/include -I/usr/include

AVRDUDE=avrdude
AVRDUDE_FLAGS=-c arduino -b 57600 -P /dev/ttyUSB0 -p atmega328p -F -u

INPOUT32_BINARIES=http://www.henning-liebenau.de/download/xlink/inpout32.zip
INPOUT32_BINARIES_MD5=http://www.henning-liebenau.de/download/xlink/inpout32.zip.md5

LIBHEADERS=\
	xlink.h \
	machine.h \
	util.h \
	error.h \
	driver/driver.h \
	driver/protocol.h \
	driver/usb.h \
	driver/parport.h \
	driver/shm.h \
	driver/serial.h

LIBSOURCES=\
	xlink.c \
	machine.c \
	util.c \
	server64.c \
	server128.c \
	kernal64.c \
	kernal128.c \
	driver/driver.c \
	driver/usb.c \
	driver/parport.c \
	driver/shm.c \
	driver/serial.c

LIBFLAGS=-DXLINK_LIBRARY_BUILD -L. -L/usr/lib -L$(PREFIX)/lib
LIBEXT=so

UNAME=$(shell uname)
MD5SUM=md5sum

ifeq ($(UNAME), Darwin)
  LIBEXT=dylib
  MD5SUM=md5 -r
endif

ifneq ($(PREFIX), /usr)
    SYSCONFDIR=$(PREFIX)/etc
endif

all: linux cbm
cbm: bootstrap
linux: xlink udev
win32: xlink.exe
macosx: xlink

bootstrap: bootstrap-c64 bootstrap-c128
bootstrap-c64: bootstrap-c64.txt
bootstrap-test-c64: bootstrap-test-c64.prg
bootstrap-c128: bootstrap-c128.txt
bootstrap-test-c128: bootstrap-test-c128.prg

testsuite: testsuite.c range.c
	$(CC) -o testsuite testsuite.c range.c

test: testsuite
	./testsuite

libxlink.$(LIBEXT): $(LIBHEADERS) $(LIBSOURCES)
	$(CC) $(CFLAGS) $(LIBFLAGS) -shared -fPIC \
		-o libxlink.$(LIBEXT) $(LIBSOURCES) -lusb-1.0

xlink: libxlink.$(LIBEXT) client.c client.h range.c range.h help.c
	$(CC) $(CFLAGS) -o xlink client.c range.c -L. -lxlink

xlink.res.o: xlink.rc
	$(MINGW32-WINDRES) -i xlink.rc -o xlink.res.o

xlink.dll: $(LIBHEADERS) $(LIBSOURCES) inpout32.dll xlink.res.o
	$(MINGW32-GCC) $(MINGW32-CFLAGS) -DXLINK_LIBRARY_BUILD -L. -L/usr/$(MINGW32)/lib \
		-static-libgcc -Wl,--enable-stdcall-fixup -shared \
		-o xlink.dll $(LIBSOURCES) xlink.res.o -lusb-1.0 -linpout32

xlink.exe: xlink.dll client.c client.h range.c range.h help.c xlink.lib-clean
	$(MINGW32-GCC) $(MINGW32-CFLAGS) -static-libgcc -o xlink.exe \
		client.c range.c -L. -L/usr/$(MINGW32)/lib -lxlink

xlink.lib: xlink.dll
	dos2unix tools/make-msvc-lib.sh
	sh tools/make-msvc-lib.sh

xlink.lib-clean:
	[ -f xlink.lib ] && rm -v xlink.lib || true

inpout32.dll:
	wget -4 $(INPOUT32_BINARIES) && \
	wget -4 $(INPOUT32_BINARIES_MD5) && \
	md5sum -c inpout32.zip.md5 && \
	unzip -d inpout32 inpout32.zip && \
	cp inpout32/Win32/inpout32.h . && \
	cp inpout32/Win32/inpout32.dll . && \
	chmod +x inpout32.dll

tools/make-server: tools/make-server.c
	$(CC) $(CFLAGS) -o tools/make-server tools/make-server.c

server64.c: tools/make-server server.h server64.asm loader.asm
	$(KASM) :target=c64 :pc=257 -o base server64.asm  # 257 = 0101
	$(KASM) :target=c64 :pc=513 -o high server64.asm  # 513 = 0201
	$(KASM) :target=c64 :pc=258 -o low  server64.asm  # 258 = 0102
	(let size=$$(stat --format=%s base)-2 && $(KASM) :size="$$size" \
		:target=c64 -o loader loader.asm)
	tools/make-server c64 base low high loader > server64.c
	rm -v base low high loader

server128.c: tools/make-server server.h server128.asm loader.asm
	$(KASM) :target=c128 :pc=257 -o base server128.asm  # 257 = 0101
	$(KASM) :target=c128 :pc=513 -o high server128.asm  # 513 = 0201
	$(KASM) :target=c128 :pc=258 -o low  server128.asm  # 258 = 0102
	(let size=$$(stat --format=%s base)-2 && $(KASM) :size="$$size" \
		:target=c128 -o loader loader.asm)
	tools/make-server c128 base low high loader > server128.c
	rm -v base low high loader

tools/make-kernal: tools/make-kernal.c
	$(CC) $(CFLAGS) -o tools/make-kernal tools/make-kernal.c

kernal64.c: tools/make-kernal tools/make-kernal.c kernal64.asm
	$(KASM) -binfile :target=c64 -o kernal64.bin kernal64.asm | \
	grep make-kernal | \
	sh -x > kernal64.c && \
	rm kernal64.bin

kernal128.c: tools/make-kernal tools/make-kernal.c kernal128.asm
	$(KASM) -binfile :target=c128 -o kernal128.bin kernal128.asm | \
	grep make-kernal | \
	sh -x > kernal128.c && \
	rm kernal128.bin

tools/make-bootstrap: tools/make-bootstrap.c
	$(CC) $(CFLAGS) -o tools/make-bootstrap tools/make-bootstrap.c

bootstrap-c64.txt: tools/make-bootstrap bootstrap.asm server.h
	$(KASM) :target=c64 -o bootstrap-c64.prg bootstrap.asm && \
	tools/make-bootstrap c64 bootstrap-c64.prg > bootstrap-c64.txt

bootstrap-test-c64.prg: bootstrap-c64.txt
	petcat -w2 -o bootstrap-test-c64.prg -- bootstrap-c64.txt

bootstrap-c128.txt: tools/make-bootstrap bootstrap.asm
	$(KASM) :target=c128 -o bootstrap-c128.prg bootstrap.asm && \
	tools/make-bootstrap c128 bootstrap-c128.prg > bootstrap-c128.txt

bootstrap-test-c128.prg: bootstrap-c128.txt
	petcat -w70 -o bootstrap-test-c128.prg -- bootstrap-c128.txt

tools/make-help: tools/make-help.c
	$(CC) $(CFLAGS) -o tools/make-help tools/make-help.c

help.c: tools/make-help help.txt
	tools/make-help help.txt > help.c

udev: etc/udev/rules.d/10-xlink.rules

etc/udev/rules.d/10-xlink.rules: tools/make-udev-rules.sh
	 tools/make-udev-rules.sh > etc/udev/rules.d/10-xlink.rules

firmware: driver/at90usb162/xlink.c driver/at90usb162/xlink.h
	(cd driver/at90usb162 && \
		make XLINK_SERIAL=$(XLINK_SERIAL) && \
		echo -e "\nFIRMWARE SERIAL NUMBER: $(XLINK_SERIAL)")

firmware-clean:
	(cd driver/at90usb162 && make clean)

firmware-install: firmware
	(cd driver/at90usb162 && make dfu)

servant64: driver/servant64/xlink.c driver/servant64/xlink.h
	(cd driver/servant64 && make)

servant64-clean:
	(cd driver/servant64 && make clean)

servant64-install: servant64
	$(AVRDUDE)  $(AVRDUDE_FLAGS) -U flash:w:driver/servant64/xlink.hex:i

server64.prg: /usr/bin/xlink
	xlink server -Mc64 server64.prg

server128.prg: /usr/bin/xlink
	xlink server -Mc128 server128.prg

xlink.d64: server64.prg server128.prg bootstrap-test-c64.prg bootstrap-test-c128.prg
	c1541 -format xlink,12 d64 xlink.d64 8 \
		-attach xlink.d64 8 \
		-write bootstrap-test-c64.prg bootstrap64 \
		-write server64.prg server64 \
		-write bootstrap-test-c128.prg bootstrap128 \
		-write server128.prg server128

install: xlink cbm
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 xlink $(DESTDIR)$(PREFIX)/bin

	install -d $(DESTDIR)$(PREFIX)/lib/
	install -m644 libxlink.$(LIBEXT) $(DESTDIR)$(PREFIX)/lib/

	install -d $(DESTDIR)$(PREFIX)/include/
	install -m644 xlink.h $(DESTDIR)$(PREFIX)/include/

ifeq ($(UNAME), Linux)
		install -m644 -D etc/udev/rules.d/10-xlink.rules \
			$(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true
endif

	install -d $(DESTDIR)$(SYSCONFDIR)/bash_completion.d/
	install -m644 etc/bash_completion.d/xlink \
			$(DESTDIR)$(SYSCONFDIR)/bash_completion.d/

uninstall:
	rm -v $(DESTDIR)$(PREFIX)/bin/xlink || true
	rm -v $(DESTDIR)$(PREFIX)/lib/libxlink.$(LIBEXT) || true
	rm -v $(DESTDIR)$(PREFIX)/include/xlink.h || true

ifeq ($(UNAME), Linux)
		rm -v $(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true
endif

	rm -v $(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

clean: firmware-clean servant64-clean
	[ -f testsuite ] && rm -vf testsuite || true
	[ -f libxlink.so ] && rm -vf libxlink.so || true
	[ -f libxlink.dylib ] && rm -vf libxlink.dylib || true
	[ -f xlink.dll ] && rm -vf xlink.dll || true
	[ -f xlink.lib ] && rm -vf xlink.lib || true
	[ -f xlink.res.o ] && rm -vf xlink.res.o || true
	[ -f xlink ] && rm -vf xlink || true
	[ -f xlink.exe ] && rm -vf xlink.exe || true
	[ -f server64.c ] && rm -vf server64.c || true
	[ -f server128.c ] && rm -vf server128.c || true
	[ -f kernal64.c ] && rm -vf kernal64.c || true
	[ -f kernal128.c ] && rm -vf kernal128.c || true
	[ -f help.c ] && rm -vf help.c || true
	[ -f bootstrap-c64.txt ] && rm -vf bootstrap-c64.txt || true
	[ -f bootstrap-c64.prg ] && rm -vf bootstrap-c64.prg || true
	[ -f bootstrap-c128.txt ] && rm -vf bootstrap-c128.txt || true
	[ -f bootstrap-c128.prg ] && rm -vf bootstrap-c128.prg || true
	[ -f bootstrap-test-c64.prg ] && rm -vf bootstrap-test-c64.prg || true
	[ -f bootstrap-test-c128.prg ] && rm -vf bootstrap-test-c128.prg || true
	[ -f tools/make-bootstrap ] && rm -vf tools/make-bootstrap || true
	[ -f tools/make-server ] && rm -vf tools/make-server || true
	[ -f tools/make-kernal ] && rm -vf tools/make-kernal || true
	[ -f tools/make-help ] && rm -vf tools/make-help || true
	[ -f etc/udev/rules.d/10-xlink.rules ] && rm -v etc/udev/rules.d/10-xlink.rules || true
	[ -f log ] && rm -v log || true

distclean: clean
	rm -rf inpout{32,x64}* || true
	rm -rf libusb-1.0* || true

release: distclean
	git archive --prefix=xlink-$(VERSION)/ -o ../xlink-$(VERSION).tar.gz HEAD && \
	$(MD5SUM) ../xlink-$(VERSION).tar.gz > ../xlink-$(VERSION).tar.gz.md5

macosx-package: ../xlink-$(VERSION)-macosx.tar.gz

../xlink-$(VERSION)-macosx.tar.gz: macosx
	make DESTDIR=stage PREFIX=/usr/local install && \
	(cd stage && tar vczf ../../xlink-$(VERSION)-macosx.tar.gz .) && \
	rm -rf stage && \
	$(MD5SUM) ../xlink-$(VERSION)-macosx.tar.gz > \
		../xlink-$(VERSION)-macosx.tar.gz.md5

msi: xlink.exe xlink.dll libusb-1.0.dll inpout32.dll xlink.wxs
	wixl --arch x86 --define VERSION=$(VERSION) -o ../xlink-$(VERSION).msi xlink.wxs && \
	$(MD5SUM) ../xlink-$(VERSION).msi > \
		../xlink-$(VERSION).msi.md5
	rm -rf libusb-1.0.dll

libusb-1.0.26-binaries.7z:
	wget https://github.com/libusb/libusb/releases/download/v1.0.26/libusb-1.0.26-binaries.7z

libusb-1.0.26-binaries: libusb-1.0.26-binaries.7z
	7z x -y libusb-1.0.26-binaries.7z

libusb-1.0.dll: libusb-1.0.26-binaries
	cp libusb-1.0.26-binaries/VS2015-Win32/dll/libusb-1.0.dll .
