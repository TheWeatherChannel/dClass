CC=gcc
CFLAGS=-O3 -Wall
CLIENT_OBJS=dtree_core.o openddr_client.o dclass_file.o dtree_mem.o dtree_util.o dclass_client.o
LIBFLAGS=-lrt
VARNISH_SRC=/some/path/varnish/varnish-3.0.1
VCFLAGS=$(CFLAGS) -I$(VARNISH_SRC)/include -I$(VARNISH_SRC)
PYTHON=python
VMODPY=$(VARNISH_SRC)/lib/libvmod_std/vmod.py


all:			dclass_client
lib:			CFLAGS=-O3 -Wall -fPIC
lib:			clean libdclass.so
varnish:		CFLAGS=-O3 -Wall -fPIC
varnish:		clean libvmod_dclass.so

dclass_client:		$(CLIENT_OBJS) main.o
			$(CC) -o dclass_client $(CLIENT_OBJS) main.o $(LIBFLAGS)

libdclass.so:		$(CLIENT_OBJS)
			$(CC) -shared $(CLIENT_OBJS) -o libdclass.so

%.o:			%.c dtree_client.h openddr_client.h dclass_client.h
			$(CC) $(CFLAGS) -c $< -o $@

libvmod_dclass.so:	$(CLIENT_OBJS) vcc_if.o vmod_dclass.o
			$(CC) -shared $(CLIENT_OBJS) vcc_if.o vmod_dclass.o -o libvmod_dclass.so

vcc_if.o:		vcc_if.c vcc_if.h
			$(CC) $(VCFLAGS) -c vcc_if.c -o vcc_if.o

vmod_dclass.o:		vmod_dclass.c
			$(CC) $(VCFLAGS) -c vmod_dclass.c -o vmod_dclass.o

vcc_if.%:		vmod_dclass.vcc
			$(PYTHON) $(VMODPY) vmod_dclass.vcc

clean:
			rm -rf *.o dclass_client vcc_if.c vcc_if.h *.so out
