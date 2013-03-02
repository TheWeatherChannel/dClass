CC=gcc
CFLAGS=-O3 -Wall -fPIC -m$(ARCH)
LDFLAGS=
OS=linux
#OS=darwin
ARCH=$(shell getconf LONG_BIT)
JAVAI=$(OS)$(ARCH)
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVAI) -I../src
JAVAC=javac
JAVAH=javah
JAVA=java
RM=rm -rf
CP=cp
SEP=/
OUTPRE=lib
OUTEXT=so
#OUTEXT=dylib
OUT=$(PREOUT)dclassjava.$(OUTEXT)

all:			$(OUT)

$(OUT):			libdclass.a dclass_java.o dclass/dClass.class dclass/dClassLoader.class
			$(CC) -shared $(LDFLAGS) -o $(OUT) dclass_java.o libdclass.a
			$(CP) $(OUT) lib$(SEP)$(PREOUT)dclassjava$(OS)$(ARCH).$(OUTEXT)

libdclass.a:		
			$(MAKE) -C ../src/ -f ../src/Makefile "CFLAGS=$(CFLAGS)" lib
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
