# Microsoft Developer Studio Project File - Name="pscp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=pscp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pscp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pscp.mak" CFG="pscp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pscp - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "pscp - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pscp - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib gdi32.lib imm32.lib shell32.lib user32.lib winmm.lib winspool.lib /nologo /subsystem:console /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "pscp - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib gdi32.lib imm32.lib shell32.lib user32.lib winmm.lib winspool.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "pscp - Win32 Release"
# Name "pscp - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\.\be_none.c
# End Source File
# Begin Source File

SOURCE=..\..\.\cmdline.c
# End Source File
# Begin Source File

SOURCE=..\..\.\console.c
# End Source File
# Begin Source File

SOURCE=..\..\.\int64.c
# End Source File
# Begin Source File

SOURCE=..\..\.\logging.c
# End Source File
# Begin Source File

SOURCE=..\..\.\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\noise.c
# End Source File
# Begin Source File

SOURCE=..\..\.\pageantc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\portfwd.c
# End Source File
# Begin Source File

SOURCE=..\..\.\pproxy.c
# End Source File
# Begin Source File

SOURCE=..\..\.\proxy.c
# End Source File
# Begin Source File

SOURCE=..\..\.\scp.c
# End Source File
# Begin Source File

SOURCE=..\..\.\settings.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sftp.c
# End Source File
# Begin Source File

SOURCE=..\..\.\ssh.c

!IF  "$(CFG)" == "pscp - Win32 Release"

!ELSEIF  "$(CFG)" == "pscp - Win32 Debug"

# ADD CPP /Zi

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\.\sshaes.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshblowf.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshbn.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshcrc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshcrcda.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshdes.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshdh.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshdss.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshmd5.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshpubk.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshrand.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshrsa.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshsh512.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshsha.c
# End Source File
# Begin Source File

SOURCE=..\..\.\sshzlib.c
# End Source File
# Begin Source File

SOURCE=..\..\.\tree234.c
# End Source File
# Begin Source File

SOURCE=..\..\.\version.c
# End Source File
# Begin Source File

SOURCE=..\..\.\wildcard.c
# End Source File
# Begin Source File

SOURCE=..\..\.\windefs.c
# End Source File
# Begin Source File

SOURCE=..\..\.\winmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\winnet.c
# End Source File
# Begin Source File

SOURCE=..\..\.\winsftp.c
# End Source File
# Begin Source File

SOURCE=..\..\.\winstore.c
# End Source File
# Begin Source File

SOURCE=..\..\.\x11fwd.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\.\int64.h
# End Source File
# Begin Source File

SOURCE=..\..\.\misc.h
# End Source File
# Begin Source File

SOURCE=..\..\.\network.h
# End Source File
# Begin Source File

SOURCE=..\..\.\proxy.h
# End Source File
# Begin Source File

SOURCE=..\..\.\psftp.h
# End Source File
# Begin Source File

SOURCE=..\..\.\putty.h
# End Source File
# Begin Source File

SOURCE=..\..\.\puttymem.h
# End Source File
# Begin Source File

SOURCE=..\..\.\puttyps.h
# End Source File
# Begin Source File

SOURCE=..\..\.\sftp.h
# End Source File
# Begin Source File

SOURCE=..\..\.\ssh.h
# End Source File
# Begin Source File

SOURCE=..\..\.\storage.h
# End Source File
# Begin Source File

SOURCE=..\..\.\tree234.h
# End Source File
# Begin Source File

SOURCE=..\..\.\winhelp.h
# End Source File
# Begin Source File

SOURCE=..\..\.\winstuff.h
# End Source File
# Begin Source File

SOURCE=..\..\charset\charset.h
# End Source File
# Begin Source File

SOURCE=..\..\mac\macstuff.h
# End Source File
# Begin Source File

SOURCE=..\..\unix\unix.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\.\scp.ico
# End Source File
# Begin Source File

SOURCE=..\..\.\scp.rc
# End Source File
# End Group
# End Target
# End Project
