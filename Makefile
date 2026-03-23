CC ?= gcc
AR ?= ar
RANLIB ?= ranlib
STRIP ?= strip

CFLAGS += -I. -Os -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables
LDFLAGS += -static -Wl,--gc-sections -s

SRC = main.c

shorkfont: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o shorkfont $(LDFLAGS)
	$(STRIP) shorkfont

PREFIX ?= /usr
BINDIR = $(PREFIX)/libexec

install: shorkfont
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 shorkfont $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/shorkfont

clean:
	rm -f shorkfont

.PHONY: install uninstall clean
