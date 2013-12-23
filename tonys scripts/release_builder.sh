#!/bin/sh
BRANCH=CVSNT_2_0_x
PATH=/cygdrive/d/cvsbin:${PATH}:/cygdrive/c/"program files"/winzip:/cygdrive/c/"Program Files"/"ISTool 4":/cygdrive/c/"Program Files"/PuTTY
rm -rf cvsnt
INCLUDE=D:/kerberos/include\;D:/Openssl/Include
LIB=D:/kerberos/lib\;D:/Openssl/Lib
cvs -d :pserver:tmh@cvs.cvsnt.org:/usr/local/cvs co -r $BRANCH cvsnt
devenv /rebuild release cvsnt/cvsnt.sln
BUILD=`cvsnt/winrel/cvs.exe -v | grep "(CVSNT)" | sed 's/.*CVSNT) \([^ ]*\).*/\1/'`
echo $BUILD
echo $BUILD >cvsnt/doc/version.inc
cd cvsnt/doc
build.bat cvs
cd ../..
rm -f ../*
rm -f ../pdb/*
cp cvsnt/winrel/*.exe ..
cp cvsnt/winrel/*.dll ..
cp cvsnt/winrel/cvsnt.cpl ..
cp cvsnt/*.ini ..
cp cvsnt/src/infolib.h ..
cp cvsnt/ca.pem ..
cp cvsnt/COPYING ..
cp cvsnt/winrel/*.pdb ../pdb
cp cvsnt/doc/*.chm ..
cd ..
wzzip cvsnt-${BUILD}-bin.zip *.dll *.exe *.cpl *.ini ca.pem infolib.h COPYING
wzzip cvsnt-${BUILD}-pdb.zip pdb
cp keep/*.* .
istool -compile "release builder"/cvsnt/cvsnt.iss
echo ${BUILD} >release
#cp *.zip cvsnt-${BUILD}.exe /cygdrive/p/NProject
pscp -i d:/tony.ppk *.zip cvsnt-${BUILD}.exe release tmh@sisko:/home/tmh/cvs
