copy ..\winrel\*.exe \dev\cvsnt-sjis\bin
copy ..\winrel\*.dll \dev\cvsnt-sjis\bin
copy ..\winrel\*.cpl \dev\cvsnt-sjis\bin
copy ..\winrel\*.cpl %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32\setuid2.dll
copy ..\src\infolib.h \dev\cvsnt-sjis\bin
copy ..\protocol_map.ini \dev\cvsnt-sjis\bin
copy ..\ca.pem \dev\cvsnt-sjis\bin
copy ..\COPYING \dev\cvsnt-sjis\bin
regtlib \dev\cvsnt-sjis\bin\cvs.exe
pause
