CC ?= gcc
AR ?= ar
RANLIB ?= ranlib
STRIP ?= strip

CFLAGS += -I. -Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables
LDFLAGS += -static -Wl,--gc-sections -s

SRC = main.c

shorkcol: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o shorkcol $(LDFLAGS)
	$(STRIP) shorkcol

PREFIX ?= /usr
BINDIR = $(PREFIX)/libexec

install: shorkcol
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 shorkcol $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/shorkcol

clean:
	rm -f shorkcol

.PHONY: install uninstall clean
