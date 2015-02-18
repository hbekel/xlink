PREFIX=/usr
SYSCONFDIR=/etc
KASM=java -jar /usr/share/kickassembler/KickAss.jar
GCC-MINGW32=i686-w64-mingw32-gcc

VERSION=0.9
USB_MANUFACTURER="Henning Bekel <h.bekel@googlemail.com>"
USB_PRODUCT="XLink USB Adapter"

USB_VID:=$(USB_VID)
USB_PID:=$(USB_PID)
USB_SERIAL:=$(USB_SERIAL)

GCC=gcc
CFLAGS=-DUSB_VID=$(USB_VID) -DUSB_PID=$(USB_PID) -DCLIENT_VERSION="$(VERSION)" -std=gnu99 -Wall -O3 -I.

INPOUT32=http://www.highrez.co.uk/scripts/download.asp?package=InpOutBinaries

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
	kernal.c \
	driver/driver.c \
	driver/usb.c \
	driver/parport.c

LIBFLAGS=-DXLINK_LIBRARY_BUILD -L.

all: linux c64
c64: bootstrap
linux: xlink udev
win32: xlink.exe
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

xlink.dll: $(LIBHEADERS) $(LIBSOURCES) inpout32
	$(GCC-MINGW32) $(CFLAGS) $(LIBFLAGS) -static-libgcc -Wl,--enable-stdcall-fixup -shared \
		-o xlink.dll $(LIBSOURCES) -lusb-1.0 -linpout32

xlink.exe: xlink.dll client.c client.h disk.c disk.h range.c range.h
	$(GCC-MINGW32) $(CFLAGS) -static-libgcc -o xlink.exe client.c disk.c range.c -L. -lxlink

inpout32:
	wget -O inpout32.zip $(INPOUT32) && \
	unzip -d inpout32 inpout32.zip && \
	cp inpout32/Win32/inpout32.h . && \
	cp inpout32/Win32/inpout32.dll . && \
	chmod +x inpout32.dll

tools/make-extension: tools/make-extension.c
	$(GCC) $(CFLAGS) -o tools/make-extension tools/make-extension.c

extensions.c: tools/make-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | \
	grep make-extension | \
	sh > extensions.c && rm extensions.bin

tools/make-server: tools/make-server.c
	$(GCC) $(CFLAGS) -o tools/make-server tools/make-server.c

server.c: tools/make-server server.h server.asm loader.asm 
	$(KASM) :pc=257 -o base server.asm  # 257 = 0101
	$(KASM) :pc=513 -o high server.asm  # 513 = 0201
	$(KASM) :pc=258 -o low  server.asm  # 258 = 0102
	(let size=$$(stat --format=%s base)-2 && $(KASM) :size="$$size" -o loader loader.asm)
	tools/make-server base low high loader > server.c
	rm -v base low high loader

tools/make-kernal: tools/make-kernal.c
	$(GCC) $(CFLAGS) -o tools/make-kernal tools/make-kernal.c

kernal.c: tools/make-kernal tools/make-kernal.c kernal.asm
	$(KASM) -binfile -o kernal.bin kernal.asm | \
	grep make-kernal | \
	sh -x > kernal.c && \
	rm kernal.bin

tools/make-bootstrap: tools/make-bootstrap.c
	$(GCC) $(CFLAGS) -o tools/make-bootstrap tools/make-bootstrap.c

bootstrap.txt: tools/make-bootstrap bootstrap.asm
	$(KASM) -o bootstrap.prg bootstrap.asm && \
	tools/make-bootstrap bootstrap.prg > bootstrap.txt && \
	rm -v bootstrap.prg

udev: etc/udev/rules.d/10-xlink.rules

etc/udev/rules.d/10-xlink.rules: tools/make-udev-rules.sh
	 tools/make-udev-rules.sh > etc/udev/rules.d/10-xlink.rules

firmware: driver/at90usb162/xlink.c driver/at90usb162/xlink.h
	(cd driver/at90usb162 && \
		make USB_PID=$(USB_PID) \
		     USB_VID=$(USB_VID) \
		     USB_SERIAL=$(USB_SERIAL) \
		     USB_MANUFACTURER=$(USB_MANUFACTURER) \
		     USB_PRODUCT=$(USB_PRODUCT) && \
		echo -e "\nFIRMWARE SERIAL NUMBER: $(USB_SERIAL)")

firmware-clean:
	(cd driver/at90usb162 && make clean)

firmware-install: firmware
	(cd driver/at90usb162 && make dfu)

install: xlink c64
	install -m755 -D xlink $(DESTDIR)$(PREFIX)/bin/xlink
	install -m644 -D libxlink.so $(DESTDIR)$(PREFIX)/lib/libxlink.so
	install -m644 -D xlink.h $(DESTDIR)$(PREFIX)/include/xlink.h

	install -m644 -D etc/udev/rules.d/10-xlink.rules \
			$(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true

	install -m644 -D etc/bash_completion.d/xlink \
			$(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

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
	[ -f kernal.c ] && rm -v kernal.c || true
	[ -f bootstrap.txt ] && rm -v bootstrap.txt || true
	[ -f tools/make-extension ] && rm -v tools/make-extension || true
	[ -f tools/make-bootstrap ] && rm -v tools/make-bootstrap || true
	[ -f tools/make-server ] && rm -v tools/make-server || true
	[ -f tools/make-kernal ] && rm -v tools/make-kernal || true
	[ -f etc/udev/rules.d/10-xlink.rules ] && rm -v etc/udev/rules.d/10-xlink.rules || true
	[ -f log ] && rm -v log || true

distclean: clean
	(yes | rm -r inpout32*) || true

release: distclean
	git archive --prefix=xlink-$(VERSION)/ -o ../xlink-$(VERSION).tar.gz HEAD && \
	md5sum ../xlink-$(VERSION).tar.gz > ../xlink-$(VERSION).tar.gz.md5

haste: distclean
	(git gc && cd .. && tar vczf /cygdrive/f/xlink.tar.gz xlink)
