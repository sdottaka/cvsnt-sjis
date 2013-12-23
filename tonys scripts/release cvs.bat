copy ..\winrel\*.exe c:\dev\cvsnt-sjis\bin
copy ..\winrel\*.dll c:\dev\cvsnt-sjis\bin
copy ..\winrel\*.cpl c:\dev\cvsnt-sjis\bin
copy ..\winrel\*.cpl %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32\setuid2.dll
copy ..\src\infolib.h c:\dev\cvsnt-sjis\bin
copy ..\protocol_map.ini c:\dev\cvsnt-sjis\bin
copy ..\ca.pem c:\dev\cvsnt-sjis\bin
copy ..\COPYING c:\dev\cvsnt-sjis\bin
regtlib c:\dev\cvsnt-sjis\bin\cvs.exe
pause
