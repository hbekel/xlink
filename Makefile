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

#all: linux c64
all: win32 c64
c64: server kernal bootstrap
linux: xlink udev
win32: xlink.exe
server: xlink-server.prg
kernal: commodore/kernal-901227-03.rom xlink-kernal.rom
bootstrap: bootstrap.txt

libxlink.so: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC) $(CFLAGS) -shared -fPIC \
		-Wl,-init,libxlink_initialize,-fini,libxlink_finalize\
		-o libxlink.so $(LIBSOURCES) -lusb-1.0

xlink: libxlink.so client.c client.h disk.c disk.h
	$(GCC) $(CFLAGS) -o xlink client.c disk.c -L. -lxlink -lreadline

xlink.dll: $(LIBHEADERS) $(LIBSOURCES)
	$(GCC-MINGW32) $(CFLAGS) -shared -o xlink.dll $(LIBSOURCES) -lusb-1.0

xlink.exe: xlink.dll client.c client.h disk.c disk.h 
	$(GCC-MINGW32) $(CFLAGS) -o xlink.exe client.c disk.c -L. -lxlink

extensions.c: tools/make-extension extensions.asm
	$(KASM) -binfile -o extensions.bin extensions.asm | \
	grep make-extension | \
	sh > extensions.c && rm extensions.bin

tools/make-extension: tools/make-extension.c
	$(GCC) $(CFLAGS) -o tools/make-extension tools/make-extension.c

tools/make-bootstrap: tools/make-bootstrap.c
	$(GCC) $(CFLAGS) -o tools/make-bootstrap tools/make-bootstrap.c

bootstrap.txt: tools/make-bootstrap bootstrap.asm
	$(KASM) -o bootstrap.prg bootstrap.asm | grep 'make-bootstrap' | \
	sh -x > bootstrap.txt && \
	rm -v bootstrap.prg

xlink-server.prg: server.h server.asm 
	$(KASM) -o xlink-server.prg server.asm

xlink-kernal.rom: server.h kernal.asm
	cp commodore/kernal-901227-03.rom xlink-kernal.rom && \
	$(KASM) -binfile kernal.asm | grep dd | sh -x >& /dev/null && rm -v kernal.bin

udev:
	echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="$(subst 0x,,$(USB_VID))", ATTRS{idProduct}=="$(subst 0x,,$(USB_PID))", MODE:="0666", SYMLINK+="xlink"' > etc/udev/rules.d/10-xlink.rules
	echo 'SUBSYSTEMS=="usb", ATTR{idVendor}=="03eb", ATTR{idProduct}=="2ffa", SYMLINK+="dfu", MODE:="666"' >> etc/udev/rules.d/10-xlink.rules

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
	install -m644 -D xlink-server.prg $(DESTDIR)$(PREFIX)/share/xlink/xlink-server.prg
	install -m644 -D xlink-kernal.rom $(DESTDIR)$(PREFIX)/share/xlink/xlink-kernal.rom
	install -m644 -D bootstrap.txt $(DESTDIR)$(PREFIX)/share/xlink/xlink-bootstrap.txt

	install -m644 -D etc/udev/rules.d/10-xlink.rules \
			$(DESTDIR)$(SYSCONFDIR)/udev/rules.d/10-xlink.rules || true

	install -m644 -D etc/bash_completion.d/xlink \
			$(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

	udevadm control --reload-rules

uninstall:
	rm -v $(DESTDIR)$(PREFIX)/bin/xlink
	rm -v $(DESTDIR)$(PREFIX)/lib/libxlink.so
	rm -v $(DESTDIR)$(PREFIX)/include/xlink.h
	rm -rv $(DESTDIR)$(PREFIX)/share/xlink
	[ -f $(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink ] && \
		rm -v $(DESTDIR)$(SYSCONFDIR)/bash_completion.d/xlink || true

clean: firmware-clean
	[ -f libxlink.so ] && rm -v libxlink.so || true
	[ -f xlink.dll ] && rm -v xlink.dll || true
	[ -f xlink ] && rm -v xlink || true
	[ -f xlink.exe ] && rm -v xlink.exe || true
	[ -f extensions.c ] && rm -v extensions.c || true
	[ -f xlink-server.prg ] && rm -v xlink-server.prg || true
	[ -f xlink-kernal.rom ] && rm -v xlink-kernal.rom || true
	[ -f bootstrap.txt ] && rm -v bootstrap.txt || true
	[ -f tools/make-extension ] && rm -v tools/make-extension || true
	[ -f tools/make-bootstrap ] && rm -v tools/make-bootstrap || true
	[ -f etc/udev/rules.d/10-xlink.rules ] && rm -v etc/udev/rules.d/10-xlink.rules || true
	[ -f log ] && rm -v log || true

release: clean
	git archive --prefix=xlink-$(VERSION)/ -o ../xlink-$(VERSION).tar.gz HEAD

haste: clean
	(cd .. && tar vczf /cygdrive/f/xlink.tar.gz xlink)
