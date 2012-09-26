PREFIX=/usr/local/
BINDIR=$(PREFIX)/bin
CFLAGS=-Wall -Werror -g
LDFLAGS=
OS=$(shell uname -s | tr A-Z a-z)
INSTALL=install

TARG=fnsproxy
MOFILE=main.o
VERS=version.h
OFILES=\
	net.o\
	srv.o\
	event.o\
	times.o\
	dlist.o\
	vector.o\
	range.o\
	radix.o\
	geo.o\
	dns.o\
	errdsp.o

all: $(VERS) $(TARG)
.PHONY: all

$(VERS):
	sh version.h.sh

$(TARG): $(OFILES) $(MOFILE)
	$(LINK.o) -o $@ $^ $(LDLIBS)

install: $(BINDIR) $(BINDIR)/$(TARG)
.PHONY: install

$(BINDIR):
	$(INSTALL) -d $@

$(BINDIR)/%: %
	$(INSTALL) $< $@

CLEANFILES:=$(CLEANFILES) $(TARG)

$(OFILES) $(MOFILE): $(HFILES)

clean:
	rm -f *.o $(CLEANFILES) version.h
.PHONY: clean
