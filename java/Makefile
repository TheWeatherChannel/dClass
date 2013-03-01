CC=gcc
CFLAGS=-O3 -Wall -fPIC
LFLAGS=
ARCH=linux
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(ARCH) -I../src
JAVAC=javac
JAVAH=javah
JAVA=java
RM=rm -rf
CP=cp
SEP=/
OUT=libdclassjava.so

all:			libdclassjava.so

libdclassjava.so:	libdclass.a dclass_java.o dclass/dClass.class
			$(CC) -shared $(LFLAGS) -o $(OUT) dclass_java.o libdclass.a

libdclass.a:		
			$(MAKE) -C ../src/ -f ../src/Makefile lib
			$(CP) ..$(SEP)src$(SEP)libdclass.a .

dclass/dClass.class:	dclass/dClass.java
			$(JAVAC) dclass/dClass.java

dclass_java.h:		dclass/dClass.class
			$(JAVAH) -o dclass_java.h -classpath . dclass.dClass

dclass_java.o:		jni/dclass_java.c dclass_java.h
			$(CC) $(INCLUDE) $(CFLAGS) -c jni/dclass_java.c -o dclass_java.o

clean:
			$(RM) *.a *.o dclass_java.h dclass$(SEP)dClass.class $(OUT)
