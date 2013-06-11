@ECHO OFF

SET JAVA_HOME=C:\DEV\Java\jdk1.7.0
SET MINGW_PATH=C:\DEV\mingw64\bin
SET PATH=%JAVA_HOME%\bin;%MINGW_PATH%;%PATH%
SET MAKE=mingw32-make.exe

SET RM=del
SET CP=copy
SET SEP=\
SET OS=win
SET ARCH=64
SET JAVAI=-I%JAVA_HOME%\include -I%JAVA_HOME%\include\%OS%%ARCH%
SET CFLAGS=-O3 -Wall -m%ARCH% -D__USE_MINGW_ANSI_STDIO=1 -D_DTREE_NO_TIMESPEC=1
SET LDFLAGS=-Wl,--kill-at -m%ARCH%
SET OUTPRE=dclassjava
SET OUTEXT=dll

%MAKE% -f Makefile -e %1

PAUSE
