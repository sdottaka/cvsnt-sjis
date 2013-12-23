# Microsoft Developer Studio Project File - Name="gserver_protocol_mit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=gserver_protocol_mit - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "gserver_protocol_mit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "gserver_protocol_mit.mak" CFG="gserver_protocol_mit - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "gserver_protocol_mit - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gserver_protocol_mit - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "gserver_protocol_mit - Win32 Purify" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "cvsnt"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "gserver_protocol_mit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release1"
# PROP BASE Intermediate_Dir ".\Release1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release1"
# PROP Intermediate_Dir ".\Release1"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /GF /GR /YX /Fp".\Release1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /GF /GR /YX /Fp".\Release1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\gserver_protocol_mit.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\gserver_protocol_mit.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinRel\gserver_protocol_mit.dll" /incremental:no /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release1/gserver_protocol_mit.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinRel\gserver_protocol_mit.dll" /incremental:no /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release1/gserver_protocol_mit.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "gserver_protocol_mit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug1"
# PROP BASE Intermediate_Dir ".\Debug1"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug1"
# PROP Intermediate_Dir ".\Debug1"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\gserver_protocol_mit.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\gserver_protocol_mit.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\gserver_protocol_mit.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug1/gserver_protocol_mit.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\gserver_protocol_mit.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug1/gserver_protocol_mit.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "gserver_protocol_mit - Win32 Purify"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Purify"
# PROP BASE Intermediate_Dir "Purify"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Purify"
# PROP Intermediate_Dir "Purify"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "gserver_protocol_mit_EXPORTS" /D "GSS_MIT" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug1/gserver_protocol_mit.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\gserver_protocol_mit.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\gserver_protocol_mit.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\gserver_protocol_mit.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug1/gserver_protocol_mit.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\gserver_protocol_mit.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug1/gserver_protocol_mit.lib" /MACHINE:I386

!ENDIF

# Begin Target

# Name "gserver_protocol_mit - Win32 Release"
# Name "gserver_protocol_mit - Win32 Debug"
# Name "gserver_protocol_mit - Win32 Purify"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=common.c
# End Source File
# Begin Source File

SOURCE=.\gserver.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=common.h
# End Source File
# Begin Source File

SOURCE=protocol_interface.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

