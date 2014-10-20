#compile flags 
CC=gcc
CFLAGS=-O3 -Wall -fPIC
LDFLAGS=
LIBFLAGS=$(NEED_LIBRT)
RM=rm -rf

#objects
CLIENT_OBJS=dtree_core.o devicemap_client.o dclass_file.o dtree_mem.o dtree_util.o dclass_client.o

#os config
NEED_LIBRT=$(shell echo 'int main() {}' | $(CC) -x c - -lrt -o /dev/null > /dev/null 2>&1 && echo -lrt)


#targets
all:			dclass_client lib
lib:			libdclass.so libdclass.a

dclass_client:		libdclass.a main.o
			$(CC) $(LDFLAGS) -o $@ main.o libdclass.a $(LIBFLAGS)

libdclass.so:		$(CLIENT_OBJS)
			$(CC) $(LDFLAGS) -shared -o $@ $(CLIENT_OBJS)

libdclass.a:		$(CLIENT_OBJS)
			ar cr $@ $(CLIENT_OBJS)

%.o:			%.c dtree_client.h devicemap_client.h dclass_client.h
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			$(RM) *.o dclass_client *.so *.a out
