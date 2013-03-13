CC=gcc
CFLAGS=-O3 -Wall -fPIC -m$(ARCH) -D_DTREE_NO_TIMESPEC=1
LDFLAGS=-m$(ARCH)
OS=linux
#OS=darwin
ARCH=$(shell getconf LONG_BIT)
JAVAI=$(OS)
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVAI) -I../src
JAVAC=javac
JAVAH=javah
JAVA=java
JAR=jar
RM=rm -rf
CP=cp
SEP=/
OUTPRE=libdclassjava
OUTEXT=so
#OUTEXT=dylib
OUT=$(OUTPRE).$(OUTEXT)

all:			dclass.jar

dclass.jar:		dclass/dClassLoader.class dclass/dClass.class dclass/dClassMain.class $(OUT)
			$(JAR) cf dclass.jar dclass/*.class lib

$(OUT):			libdclass.a dclass_java.o
			$(CC) -shared $(LDFLAGS) -o $(OUT) dclass_java.o libdclass.a
			$(CP) $(OUT) lib$(SEP)dclassjava$(OS)$(ARCH).$(OUTEXT)

libdclass.a:		
			$(MAKE) -C ../src/ -f ../src/Makefile "CFLAGS=$(CFLAGS)" "LDFLAGS=$(LDFLAGS)" lib
			$(CP) ..$(SEP)src$(SEP)libdclass.a .

dclass/%.class:		dclass/%.java
			$(JAVAC) $<

dclass_java.h:		dclass/dClass.class
			$(JAVAH) -o dclass_java.h -classpath . dclass.dClass

dclass_java.o:		jni/dclass_java.c dclass_java.h
			$(CC) $(INCLUDE) $(CFLAGS) -c jni/dclass_java.c -o dclass_java.o

clean:
			$(RM) *.a *.o dclass.jar dclass_java.h dclass$(SEP)*.class $(OUT)

test:			all
			$(JAVA) -cp dclass.jar dclass.dClassMain
