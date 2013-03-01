@ECHO OFF

SET PATH=%PATH%;C:\DEV\MinGW\bin
SET MAKE=mingw32-make.exe

SET RM=del
SET CP=copy
SET SEP=\
SET CFLAGS=-O3 -Wall -D_GNU_SOURCE=1 -D_DTREE_NO_TIMESPEC=1
SET LFLAGS=-Wl,--kill-at
SET ARCH=win32
SET OUT=dclassjava.dll

%MAKE% -f Makefile -e %1

PAUSE
