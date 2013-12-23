# Microsoft Developer Studio Project File - Name="cvs95" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cvs95 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cvs95.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cvs95.mak" CFG="cvs95 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cvs95 - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cvs95 - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cvs95 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "cvs95_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /Zi /O2 /Op /Ob2 /I "..\windows-NT" /I "..\src" /I "..\lib" /I "..\diff" /I "..\zlib" /I "..\cvs95" /I "..\cvsgui" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "CVS95_EXPORTS" /D "CVS95" /D "WIN32" /D "HAVE_CONFIG_H" /D "POSIX" /D "CVSGUI" /D "CVSGUI_PIPE" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib mpr.lib /nologo /pdb:"..\WinRel\cvs95.pdb" /debug /machine:I386 /out:"..\WinRel\cvs95.exe" /pdbtype:sept /mapinfo:lines /opt:ref
# SUBTRACT LINK32 /pdb:none /map

!ELSEIF  "$(CFG)" == "cvs95 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
F90=fl32.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "cvs95_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\windows-NT" /I "..\src" /I "..\lib" /I "..\diff" /I "..\zlib" /I "..\cvs95" /I "..\cvsgui" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "CVS95_EXPORTS" /D "CVS95" /D "WIN32" /D "HAVE_CONFIG_H" /D "POSIX" /D "CVSGUI" /D "CVSGUI_PIPE" /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib netapi32.lib mpr.lib wsock32.lib /nologo /pdb:"..\WinDebug\cvs95.pdb" /debug /machine:I386 /out:"..\WinDebug\cvs95.exe" /pdbtype:sept /mapinfo:lines
# SUBTRACT LINK32 /pdb:none /incremental:no /map

!ENDIF 

# Begin Target

# Name "cvs95 - Win32 Release"
# Name "cvs95 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\add.c
# End Source File
# Begin Source File

SOURCE=..\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\src\admin.c
# End Source File
# Begin Source File

SOURCE=..\diff\analyze.c
# End Source File
# Begin Source File

SOURCE=..\src\annotate.c
# End Source File
# Begin Source File

SOURCE=..\lib\argmatch.c
# End Source File
# Begin Source File

SOURCE=..\src\buffer.c
# End Source File
# Begin Source File

SOURCE=..\src\chacl.c
# End Source File
# Begin Source File

SOURCE=..\src\checkin.c
# End Source File
# Begin Source File

SOURCE=..\src\checkout.c
# End Source File
# Begin Source File

SOURCE=..\src\chown.c
# End Source File
# Begin Source File

SOURCE=..\src\classify.c
# End Source File
# Begin Source File

SOURCE=..\src\client.c
# End Source File
# Begin Source File

SOURCE=..\diff\cmpbuf.c
# End Source File
# Begin Source File

SOURCE=..\src\commit.c
# End Source File
# Begin Source File

SOURCE=..\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\diff\context.c
# End Source File
# Begin Source File

SOURCE=..\zlib\crc32.c
# End Source File
# Begin Source File

SOURCE=..\src\create_adm.c
# End Source File
# Begin Source File

SOURCE=..\src\cvsrc.c
# End Source File
# Begin Source File

SOURCE=..\src\cvsrcs.c
# End Source File
# Begin Source File

SOURCE=..\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\diff\diff.c

!IF  "$(CFG)" == "cvs95 - Win32 Release"

# PROP Intermediate_Dir "Release\1"

!ELSEIF  "$(CFG)" == "cvs95 - Win32 Debug"

# PROP Intermediate_Dir "Debug\1"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\diff.c

!IF  "$(CFG)" == "cvs95 - Win32 Release"

# PROP Intermediate_Dir "Release\2"

!ELSEIF  "$(CFG)" == "cvs95 - Win32 Debug"

# PROP Intermediate_Dir "Debug\2"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\diff\diff3.c
# End Source File
# Begin Source File

SOURCE=..\diff\dir.c
# End Source File
# Begin Source File

SOURCE=..\diff\ed.c
# End Source File
# Begin Source File

SOURCE=..\src\edit.c
# End Source File
# Begin Source File

SOURCE=..\src\entries.c
# End Source File
# Begin Source File

SOURCE=..\src\error.c
# End Source File
# Begin Source File

SOURCE=..\src\expand_path.c
# End Source File
# Begin Source File

SOURCE=..\src\fileattr.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\filesubr.c"
# End Source File
# Begin Source File

SOURCE=..\src\find_names.c
# End Source File
# Begin Source File

SOURCE=..\lib\fncase.c
# End Source File
# Begin Source File

SOURCE=..\lib\fnmatch.c
# End Source File
# Begin Source File

SOURCE=..\lib\getdate.c
# End Source File
# Begin Source File

SOURCE=..\lib\getline.c
# End Source File
# Begin Source File

SOURCE=..\lib\getopt.c
# End Source File
# Begin Source File

SOURCE=..\lib\getopt1.c
# End Source File
# Begin Source File

SOURCE=..\zlib\gzio.c
# End Source File
# Begin Source File

SOURCE=..\src\hash.c
# End Source File
# Begin Source File

SOURCE=..\src\history.c
# End Source File
# Begin Source File

SOURCE=..\diff\ifdef.c
# End Source File
# Begin Source File

SOURCE=..\src\ignore.c
# End Source File
# Begin Source File

SOURCE=..\src\import.c
# End Source File
# Begin Source File

SOURCE=..\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=..\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\src\info.c
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=..\diff\io.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\library.c"
# End Source File
# Begin Source File

SOURCE=..\src\lock.c
# End Source File
# Begin Source File

SOURCE=..\src\log.c
# End Source File
# Begin Source File

SOURCE=..\src\login.c
# End Source File
# Begin Source File

SOURCE=..\src\logmsg.c
# End Source File
# Begin Source File

SOURCE=..\src\ls.c
# End Source File
# Begin Source File

SOURCE=..\src\lsacl.c
# End Source File
# Begin Source File

SOURCE=..\src\main.c
# End Source File
# Begin Source File

SOURCE=..\lib\md5.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\mkdir.c"
# End Source File
# Begin Source File

SOURCE=..\src\mkmodules.c
# End Source File
# Begin Source File

SOURCE=..\src\modules.c
# End Source File
# Begin Source File

SOURCE=..\src\myndbm.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\ndir.c"
# End Source File
# Begin Source File

SOURCE=..\src\no_diff.c
# End Source File
# Begin Source File

SOURCE=..\diff\normal.c
# End Source File
# Begin Source File

SOURCE=..\src\parseinfo.c
# End Source File
# Begin Source File

SOURCE=..\src\passwd.c
# End Source File
# Begin Source File

SOURCE=..\src\patch.c
# End Source File
# Begin Source File

SOURCE=..\src\perms.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\pwd.c"
# End Source File
# Begin Source File

SOURCE=..\src\rcs.c
# End Source File
# Begin Source File

SOURCE=..\src\rcscmds.c
# End Source File
# Begin Source File

SOURCE=..\src\recurse.c
# End Source File
# Begin Source File

SOURCE=..\lib\regex.c
# End Source File
# Begin Source File

SOURCE=..\src\release.c
# End Source File
# Begin Source File

SOURCE=..\src\remove.c
# End Source File
# Begin Source File

SOURCE=..\src\repos.c
# End Source File
# Begin Source File

SOURCE=..\src\root.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\run.c"
# End Source File
# Begin Source File

SOURCE=..\lib\savecwd.c
# End Source File
# Begin Source File

SOURCE=..\src\scramble.c
# End Source File
# Begin Source File

SOURCE=..\src\server.c
# End Source File
# Begin Source File

SOURCE=..\diff\side.c
# End Source File
# Begin Source File

SOURCE=..\lib\sighandle.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\sockerror.c"
# End Source File
# Begin Source File

SOURCE=..\src\status.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\stripslash.c"
# End Source File
# Begin Source File

SOURCE=..\src\subr.c
# End Source File
# Begin Source File

SOURCE=..\src\tag.c
# End Source File
# Begin Source File

SOURCE=..\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\src\update.c
# End Source File
# Begin Source File

SOURCE=..\diff\util.c
# End Source File
# Begin Source File

SOURCE=..\lib\valloc.c
# End Source File
# Begin Source File

SOURCE=..\src\vers_ts.c
# End Source File
# Begin Source File

SOURCE=..\src\version.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\waitpid.c"
# End Source File
# Begin Source File

SOURCE=..\src\watch.c
# End Source File
# Begin Source File

SOURCE="..\windows-NT\win32.c"
# End Source File
# Begin Source File

SOURCE=..\src\wrapper.c
# End Source File
# Begin Source File

SOURCE=..\lib\xgetwd.c
# End Source File
# Begin Source File

SOURCE=..\lib\yesno.c
# End Source File
# Begin Source File

SOURCE=..\src\zlib.c
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\buffer.h
# End Source File
# Begin Source File

SOURCE=..\src\client.h
# End Source File
# Begin Source File

SOURCE=..\diff\cmpbuf.h
# End Source File
# Begin Source File

SOURCE=..\src\cvsroott.h
# End Source File
# Begin Source File

SOURCE=..\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\diff\diff.h
# End Source File
# Begin Source File

SOURCE=..\src\edit.h
# End Source File
# Begin Source File

SOURCE=..\src\fileattr.h
# End Source File
# Begin Source File

SOURCE=..\lib\fnmatch.h
# End Source File
# Begin Source File

SOURCE=..\lib\getline.h
# End Source File
# Begin Source File

SOURCE=..\lib\getopt.h
# End Source File
# Begin Source File

SOURCE=..\src\hash.h
# End Source File
# Begin Source File

SOURCE=..\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=..\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\library.h"
# End Source File
# Begin Source File

SOURCE=..\lib\md5.h
# End Source File
# Begin Source File

SOURCE=..\src\myndbm.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\ndir.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\pwd.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\rcmd.h"
# End Source File
# Begin Source File

SOURCE=..\src\rcs.h
# End Source File
# Begin Source File

SOURCE=..\lib\regex.h
# End Source File
# Begin Source File

SOURCE=..\lib\savecwd.h
# End Source File
# Begin Source File

SOURCE=..\src\server.h
# End Source File
# Begin Source File

SOURCE=..\src\update.h
# End Source File
# Begin Source File

SOURCE=..\src\watch.h
# End Source File
# Begin Source File

SOURCE=..\zlib\zutil.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cvs95.rc
# End Source File
# Begin Source File

SOURCE="..\windows-NT\res\cvs95.rc2"
# End Source File
# End Group
# Begin Group "Cvsgui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\cvsgui\cvsgui.c
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui.h
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui_process.cpp
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui_process.h
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui_protocol.h
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui_wire.cpp
# End Source File
# Begin Source File

SOURCE=..\cvsgui\cvsgui_wire.h
# End Source File
# End Group
# End Target
# End Project
