VERSION=1.0
PREFIX=/usr
SYSCONFDIR=/etc
DESTDIR=

USB_VID:=$(USB_VID)
USB_PID:=$(USB_PID)
USB_SERIAL=`uuidgen`

GCC=gcc
CFLAGS=-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -std=gnu99 -Wall -O3 -I.

#GCC-MINGW32=i686-w64-mingw32-gcc
GCC-MINGW32=i686-pc-mingw32-gcc

KASM=java -jar /usr/share/kickassembler/KickAss.jar
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
	error.h \
	util.c \
	extension.c \
	extensions.c \
	server.c \
	driver/driver.c \
	driver/usb.c \
	driver/parport.c

LIBFLAGS=-DXLINK_LIBRARY_BUILD

#all: linux c64
all: win32 c64
c64: kernal bootstrap
linux: xlink udev
win32: xlink.exe
kernal: commodore/kernal-901227-03.rom xlink-kernal.rom
bootstrap: bootstrap.txt

testsuite: testsuite.c range.c
	$(GCC) -o testsuite testsuite.c range.c

test: testsuite
	./testsuite

libxlink.so: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC) $(CFLAGS) $(LIBFLAGS) -shared -fPIC \
		-Wl,-init,libxlink_initialize,-fini,libxlink_finalize\
		-o libxlink.so $(LIBSOURCES) -lusb-1.0

xlink: libxlink.so client.c client.h disk.c disk.h range.c range.h
	$(GCC) $(CFLAGS) -o xlink client.c disk.c range.c -L. -lxlink -lreadline

xlink.dll: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC-MINGW32) $(CFLAGS) $(LIBFLAGS) -static-libgcc -shared -o xlink.dll $(LIBSOURCES) -lusb-1.0

xlink.exe: xlink.dll client.c client.h disk.c disk.h range.c range.h
	$(GCC-MINGW32) $(CFLAGS) -static-libgcc -o xlink.exe client.c disk.c range.c -L. -lxlink

tools/make-extension: tools/make-extension.c
	$(GCC) $(CFLAGS) -o tools/make-extension tools/make-extension.c

extensions.c: tools/make-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | \
	grep make-extension | \
	sh > extensions.c && rm extensions.bin

tools/make-server: tools/make-server.c
	$(GCC) $(CFLAGS) -o tools/make-server tools/make-server.c

server.c: tools/make-server server.h server.asm 
	$(KASM) :pc=257 -o base server.asm  # 257 = $0101
	$(KASM) :pc=513 -o high server.asm  # 513 = $0201
	$(KASM) :pc=258 -o low  server.asm  # 258 = $0102
	tools/make-server base low high > server.c
	rm -v base low high

tools/make-bootstrap: tools/make-bootstrap.c
	$(GCC) $(CFLAGS) -o tools/make-bootstrap tools/make-bootstrap.c

bootstrap.txt: tools/make-bootstrap bootstrap.asm
	$(KASM) -o bootstrap.prg bootstrap.asm | grep 'make-bootstrap' | \
	sh -x > bootstrap.txt && \
	rm -v bootstrap.prg

xlink-kernal.rom: server.h kernal.asm
	cp commodore/kernal-901227-03.rom xlink-kernal.rom && \
	$(KASM) -binfile kernal.asm | grep dd | sh -x >& /dev/null && rm -v kernal.bin

udev: etc/udev/rules.d/10-xlink.rules

etc/udev/rules.d/10-xlink.rules: tools/make-udev-rules.sh
	 tools/make-udev-rules.sh > etc/udev/rules.d/10-xlink.rules

firmware: driver/at90usb162/xlink.c driver/at90usb162/xlink.h
	(cd driver/at90usb162 && \
		make USB_PID=$(USB_PID) USB_VID=$(USB_VID) USB_SERIAL=$(USB_SERIAL))

firmware-clean:
	(cd driver/at90usb162 && make clean)

firmware-install: xlink firmware
	LD_LIBRARY_PATH=. ./xlink bootloader && \
	sleep 5 && \
	(cd driver/at90usb162 && make dfu)

install: xlink c64
	install -m755 -D xlink $(DESTDIR)$(PREFIX)/bin/xlink
	install -m644 -D libxlink.so $(DESTDIR)$(PREFIX)/lib/libxlink.so
	install -m644 -D xlink.h $(DESTDIR)$(PREFIX)/include/xlink.h
	install -m644 -D xlink-kernal.rom $(DESTDIR)$(PREFIX)/share/xlink/xlink-kernal.rom
	install -m644 -D bootstrap.txt $(DESTDIR)$(PREFIX)/share/xlink/xlink-bootstrap.txt

	install -m644 -D etc/udev/rules.d/10-xlink.rules \
			$(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true

	install -m644 -D etc/bash_completion.d/xlink \
			$(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

	udevadm control --reload-rules || true
	udevadm trigger || true

uninstall:
	rm -v $(DESTDIR)$(PREFIX)/bin/xlink || true
	rm -v $(DESTDIR)$(PREFIX)/lib/libxlink.so || true
	rm -v $(DESTDIR)$(PREFIX)/include/xlink.h || true
	rm -rv $(DESTDIR)$(PREFIX)/share/xlink || true
	rm -v $(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true
	rm -v $(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

clean: firmware-clean
	[ -f testsuite ] && rm -v testsuite || true
	[ -f libxlink.so ] && rm -v libxlink.so || true
	[ -f xlink.dll ] && rm -v xlink.dll || true
	[ -f xlink ] && rm -v xlink || true
	[ -f xlink.exe ] && rm -v xlink.exe || true
	[ -f extensions.c ] && rm -v extensions.c || true
	[ -f server.c ] && rm -v server.c || true
	[ -f xlink-kernal.rom ] && rm -v xlink-kernal.rom || true
	[ -f bootstrap.txt ] && rm -v bootstrap.txt || true
	[ -f tools/make-extension ] && rm -v tools/make-extension || true
	[ -f tools/make-bootstrap ] && rm -v tools/make-bootstrap || true
	[ -f tools/make-server ] && rm -v tools/make-server || true
	[ -f etc/udev/rules.d/10-xlink.rules ] && rm -v etc/udev/rules.d/10-xlink.rules || true
	[ -f log ] && rm -v log || true

release: clean
	git archive --prefix=xlink-$(VERSION)/ -o ../xlink-$(VERSION).tar.gz HEAD

haste: clean
	(cd .. && tar vczf /cygdrive/f/xlink.tar.gz xlink)
