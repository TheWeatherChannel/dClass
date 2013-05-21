#compile flags 
CC=gcc
CFLAGS=-O3 -Wall -fPIC
RM=rm -rf

#varnish
VARNISH_SRC=/some/path/varnish/varnish-3.0.3
VCFLAGS=$(CFLAGS) -I$(VARNISH_SRC)/include -I$(VARNISH_SRC) -I../../src
PYTHON=python
VMODPY=$(VARNISH_SRC)/lib/libvmod_std/vmod.py


#targets
all:			varnish
varnish:		libvmod_dclass.so

libvmod_dclass.so:	 libdclass.a vcc_if.o vmod_dclass.o
			$(CC) -shared -o libvmod_dclass.so vcc_if.o vmod_dclass.o libdclass.a

vcc_if.o:		vcc_if.c vcc_if.h
			$(CC) $(VCFLAGS) -c vcc_if.c -o vcc_if.o

vmod_dclass.o:		vmod_dclass.c
			$(CC) $(VCFLAGS) -c vmod_dclass.c -o vmod_dclass.o

vcc_if.%:		vmod_dclass.vcc
			$(PYTHON) $(VMODPY) vmod_dclass.vcc

libdclass.a:
			$(MAKE) -C ../../src/ -f ../src/Makefile "CFLAGS=$(CFLAGS)" lib
			cp ../../src/libdclass.a .

clean:
			$(RM) *.o vcc_if.c vcc_if.h *.so *.a
