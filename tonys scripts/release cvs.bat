copy ..\winrel\*.exe c:\cvsnt-sjis\bin
copy ..\winrel\*.dll c:\cvsnt-sjis\bin
copy ..\winrel\*.cpl c:\cvsnt-sjis\bin
copy ..\winrel\*.cpl %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32
copy ..\winrel\setuid.dll %systemroot%\system32\setuid2.dll
copy ..\src\infolib.h c:\cvsnt-sjis\bin
copy ..\protocol_map.ini c:\cvsnt-sjis\bin
copy ..\ca.pem c:\cvsnt-sjis\bin
copy ..\COPYING c:\cvsnt-sjis\bin
regtlib c:\cvsnt-sjis\bin\cvs.exe


