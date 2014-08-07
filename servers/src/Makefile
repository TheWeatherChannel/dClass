#compile flags 
CC=gcc
CFLAGS=-O3 -Wall -fPIC
RM=rm -rf

#varnish
VARNISH_SRC=/some/path/varnish/varnish-3.0.3
VCFLAGS=$(CFLAGS) -I$(VARNISH_SRC)/include -I$(VARNISH_SRC) -I../../src
PYTHON=python
VMODPY=$(VARNISH_SRC)/lib/libvmod_std/vmod.py
VMODVCC=vmod_dclass.vcc

#varnish4
VARNISH4=1
VARNISH4_PREFIX=/some/path/share/varnish4
VARNISH4_SRC=/some/path/varnish/varnish-4.0.0

ifeq ($(VARNISH4),1)
  VMODPY=$(VARNISH4_PREFIX)/share/varnish/vmodtool.py
  VCFLAGS=$(CFLAGS) -I$(VARNISH4_PREFIX)/share/varnish/include -I$(VARNISH4_SRC) -I$(VARNISH4_PREFIX)/include/varnish -I../../src -D_DCLASS_VARNISH4=1
  VMODVCC=vmod_dclass4.vcc
endif

#targets
all:			varnish
varnish:		libvmod_dclass.so

libvmod_dclass.so:	libdclass.a vcc_if.o vmod_dclass.o
			$(CC) -shared -o libvmod_dclass.so vcc_if.o vmod_dclass.o libdclass.a

vcc_if.o:		vcc_if.c vcc_if.h
			$(CC) $(VCFLAGS) -c vcc_if.c -o vcc_if.o

vmod_dclass.o:		vmod_dclass.c
			$(CC) $(VCFLAGS) -c vmod_dclass.c -o vmod_dclass.o

vcc_if.%:		$(VMODVCC)
			$(PYTHON) $(VMODPY) $(VMODVCC)

libdclass.a:
			$(MAKE) -C ../../src/ -f ../src/Makefile "CFLAGS=$(CFLAGS)" lib
			cp ../../src/libdclass.a .

clean:
			$(RM) *.o vcc_if.c vcc_if.h *.so *.a *.rst
