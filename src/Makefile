#compile flags 
CC=gcc
CFLAGS=-O3 -Wall
LIBFLAGS=$(NEED_LIBRT)
RM=rm -rf

#objects
CLIENT_OBJS=dtree_core.o openddr_client.o dclass_file.o dtree_mem.o dtree_util.o dclass_client.o

#os config
NEED_LIBRT=$(shell echo 'int main() {}' | $(CC) -x c - -lrt -o /dev/null > /dev/null 2>&1 && echo -lrt)


#targets
all:			dclass_client
lib:			CFLAGS=-O3 -Wall -fPIC
lib:			clean libdclass.so libdclass.a

dclass_client:		$(CLIENT_OBJS) main.o
			$(CC) -o dclass_client $(CLIENT_OBJS) main.o $(LIBFLAGS)

libdclass.so:		$(CLIENT_OBJS)
			$(CC) -shared $(CLIENT_OBJS) -o libdclass.so

libdclass.a:		$(CLIENT_OBJS)
			ar cr libdclass.a $(CLIENT_OBJS)

%.o:			%.c dtree_client.h openddr_client.h dclass_client.h
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			$(RM) *.o dclass_client *.so *.a out
