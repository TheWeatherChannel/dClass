CC=gcc
CFLAGS=-O3 -Wall -fPIC
INCLUDE=-I. -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -I../src
JAVAC=javac
JAVAH=javah
JAVA=java
#-Wl,--kill-at

all:			libdclassjava.so

libdclassjava.so:	libdclass.a dclass_java.o dclass/dClass.class
			$(CC) -shared -o libdclassjava.so dclass_java.o libdclass.a

libdclass.a:		
			$(MAKE) -C ../src/ -f ../src/Makefile lib
			cp ../src/libdclass.a .

dclass/dClass.class:	dclass/dClass.java
			$(JAVAC) dclass/dClass.java

dclass_java.h:		dclass/dClass.class
			$(JAVAH) -o dclass_java.h -classpath . dclass.dClass

dclass_java.o:		jni/dclass_java.c dclass_java.h
			$(CC) $(INCLUDE) $(CFLAGS) -c jni/dclass_java.c -o dclass_java.o

clean:
			rm -rf *.so *.a *.o dclass_java.h dclass/dClass.class
