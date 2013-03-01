CC=gcc
CFLAGS=-O3 -Wall -fPIC
LFLAGS=
ARCH=linux
#ARCH=darwin
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(ARCH) -I../src
JAVAC=javac
JAVAH=javah
JAVA=java
RM=rm -rf
CP=cp
SEP=/
OUT=libdclassjava.so
#OUT=libdclassjava.dylib

all:			$(OUT)

$(OUT):			libdclass.a dclass_java.o dclass/dClass.class dclass/dClassLoader.class
			$(CC) -shared $(LFLAGS) -o $(OUT) dclass_java.o libdclass.a

libdclass.a:		
			$(MAKE) -C ../src/ -f ../src/Makefile lib
			$(CP) ..$(SEP)src$(SEP)libdclass.a .

dclass/%.class:	dclass/%.java
			$(JAVAC) $<

dclass_java.h:		dclass/dClass.class
			$(JAVAH) -o dclass_java.h -classpath . dclass.dClass

dclass_java.o:		jni/dclass_java.c dclass_java.h
			$(CC) $(INCLUDE) $(CFLAGS) -c jni/dclass_java.c -o dclass_java.o

clean:
			$(RM) *.a *.o dclass_java.h dclass$(SEP)*.class $(OUT)

test:			all
			$(JAVA) -cp . -Djava.library.path=. dclass.dClass "../dtrees/openddr.dtree" "Mozilla/5.0 (Linux; U; Android 2.2; en; HTC Aria A6380 Build/ERE27) AppleWebKit/540.13+ (KHTML, like Gecko) Version/3.1 Mobile Safari/524.15.0"
