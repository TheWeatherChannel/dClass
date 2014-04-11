CC=gcc
CFLAGS=-O3 -Wall -fPIC -m$(BIT) -D_DTREE_NO_TIMESPEC=1 -DDTREE_DT_PACKED_16=0
LDFLAGS=-m$(BIT)

UOS=$(shell uname -s)

OS=linux
OSI=$(OS)
BIT=$(shell getconf LONG_BIT)
#ARCH=$(shell arch)
ARCH=x86_$(BIT)
OUTEXT=so

ifeq ($(UOS),Darwin)
  OS=osx
  OSI=darwin
  OUTEXT=dylib
endif

OUTPRE=libdclassjava
OUT=$(OUTPRE).$(OUTEXT)

JAVAI=-I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(OSI)
INCLUDE=-I. -I../src $(JAVAI)

JAVAC=javac
JAVAH=javah
JAVA=java
JAR=jar

RM=rm -rf
CP=cp
SEP=/

all:			dclass.jar

lib:			$(OUT)

dclass.jar:		dclass/dClassLoader.class dclass/dClass.class dclass/dClassMain.class
			$(JAR) cvf dclass.jar dclass/*.class lib

$(OUT):			libdclass.a dclass_java.o
			$(CC) -shared $(LDFLAGS) -o $(OUT) dclass_java.o libdclass.a
			$(CP) $(OUT) lib$(SEP)dclassjava_$(OS)_$(ARCH).$(OUTEXT)

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
