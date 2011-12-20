# Makefile for LabPython LabVIEW script node extension
# Copyright (C) 2002 Rolf Kalbermatter.
# For conditions of distribution and use, see copyright notice in README 

# To compile, type:
#   make

# To install <labview>/resource/script/pytscript.* type:
#    make install

prefix = /usr/local/lv61
instdir =${prefix}/resource/script
cindir =${prefix}/cintools
pyinclude =/usr/include/python2.5

CC=gcc
CFLAGS=-DNDEBUG -fpic -O3 -Wall -Wstrict-prototypes -I. -fno-strict-aliasing -I$(cindir) -I$(pyinclude)
#CFLAGS=-g -DDEBUG -fpic -O0 -Wall -Wstrict-prototypes -I. -I$(cindir) -I$(pyinclude)

#VER=1.0
SHAREDLIB=pytscript.so
#SHAREDLIB=lvpython.so

#LDFLAGS=-L. -lz
#Non Linux should probably use '-Wl,--export-dynamic' instead of '-rdynamic'
LDSHARED=$(CC) -shared -Wl,--export-dynamic
#LDSHARED=$(CC) -rdynamic -shared
CPP=$(CC) -E

AR=ar rc
RANLIB=ranlib
TAR=tar
SHELL=/bin/sh

OBJS = pytscript.o dynlib.o
#OBJS = lvpython.o pyimport.o dynlib.o

DISTFILES = README ChangeLog Make*[a-z0-9] *.[ch] \
  pytscript.rc python.ico lvpython.dsp lvpython.dsw

all: clean install

$(SHAREDLIB): $(OBJS)
	$(LDSHARED) -o $@ $(OBJS) 

install: $(SHAREDLIB)
	-@if [ ! -d $(instdir) ]; then mkdir $(instdir); fi
	cp $(SHAREDLIB) $(instdir)
	cd $(instdir); chmod 755 $(SHAREDLIB)

uninstall:
	cd $(instdir); \
	if test -f $(SHAREDLIB); then \
	  rm -f $(SHAREDLIB); \
	fi

clean:
	rm -f *.o *~ pytscript.so

distclean:	clean

dist:
	mv Makefile Makefile~; cp -p Makefile.in Makefile
	d=pytscript-`sed -n '/VERSION "/s/.*"\(.*\)".*/\1/p' < lvpython.h`;\
	if test ! -d ../$$d; then rm -f ../$$d; ln -s `pwd` ../$$d; fi; \
	files=""; \
	for f in $(DISTFILES); do files="$$files $$d/$$f"; done; \
	cd ..; \
	mv Makefile~ Makefile

tags:	
	etags *.[ch]

depend:
	makedepend -- $(CFLAGS) -- *.[ch]

# DO NOT DELETE THIS LINE -- make depend depends on it.

lvpython.o: lvpython.h lvtypedef.h
pyimport.o: lvpython.h lvtypedef.h
