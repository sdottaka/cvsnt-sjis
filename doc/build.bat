@echo off
rmdir /q /s _tmp
mkdir _tmp
cd _tmp
xsltproc /cygdrive/d/docbook/docbook-xsl-1.65.1/htmlhelp/htmlhelp.xsl ../%1.dbk
hhc htmlhelp.hhp 
cp htmlhelp.chm ../%1.chm
cd ..
rmdir /q /s _tmp
