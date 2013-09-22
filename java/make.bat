@ECHO OFF

IF "%JAVA_HOME%" == "" (
  SET JAVA_HOME=C:\DEV\Java\jdk1.7.0
)

SET MINGW_PATH=C:\DEV\mingw64\bin
SET PATH=%JAVA_HOME%\bin;%MINGW_PATH%;%PATH%
SET MAKE=mingw32-make.exe

IF "%ProgramFiles(x86)%" == "" (
  SET BIT=32
) ELSE (
  SET BIT=64
)

SET RM=del
SET CP=copy
SET SEP=\

SET OS=win
SET ARCH=x86_%BIT%
SET OUTPRE=dclassjava
SET OUTEXT=dll

SET JAVAI="-I%JAVA_HOME%\include" "-I%JAVA_HOME%\include\%OS%32"
SET CFLAGS=-O3 -Wall -m%BIT% -D__USE_MINGW_ANSI_STDIO=1 -D_DTREE_NO_TIMESPEC=1 -DDTREE_DT_PACKED_16=0
SET LDFLAGS=-Wl,--kill-at -m%BIT%

%MAKE% -f Makefile -e %1

PAUSE
