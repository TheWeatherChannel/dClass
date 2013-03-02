@ECHO OFF

SET PATH=%PATH%;C:\DEV\mingw64\bin
SET MAKE=mingw32-make.exe

SET RM=del
SET CP=copy
SET SEP=\
SET OS=win
SET ARCH=64
SET JAVAI=win32
SET CFLAGS=-O3 -Wall -m%ARCH% -D__USE_MINGW_ANSI_STDIO=1
SET LDFLAGS=-Wl,--kill-at -m%ARCH%
SET PREOUT=
SET OUTEXT=dll

%MAKE% -f Makefile -e %1

PAUSE
