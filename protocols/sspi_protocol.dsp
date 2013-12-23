# Microsoft Developer Studio Project File - Name="sspi_protocol" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sspi_protocol - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sspi_protocol.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sspi_protocol.mak" CFG="sspi_protocol - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sspi_protocol - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sspi_protocol - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sspi_protocol - Win32 Purify" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "cvsnt"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sspi_protocol - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\sspi_protocol.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\sspi_protocol.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\sspi_protocol.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/sspi_protocol.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\sspi_protocol.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/sspi_protocol.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "sspi_protocol - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /GF /GR /YX /Fp".\Release/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /GF /GR /YX /Fp".\Release/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\sspi_protocol.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\sspi_protocol.tlb" /win32 
# ADD BASE RSC /l 2057 /d "NDEBUG" 
# ADD RSC /l 2057 /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinRel\sspi_protocol.dll" /incremental:no /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release/sspi_protocol.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinRel\sspi_protocol.dll" /incremental:no /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release/sspi_protocol.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "sspi_protocol - Win32 Purify"

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
# ADD BASE CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "sspi_PROTOCOL_EXPORTS" /D "_MBCS" /Gm /Gy /GR /YX /Fp".\Debug/sspi_protocol.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\sspi_protocol.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\sspi_protocol.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_DEBUG" 
# ADD RSC /l 2057 /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\sspi_protocol.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/sspi_protocol.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /dll /out:"..\WinDebug\sspi_protocol.dll" /incremental:yes /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/sspi_protocol.lib" /MACHINE:I386

!ENDIF

# Begin Target

# Name "sspi_protocol - Win32 Debug"
# Name "sspi_protocol - Win32 Release"
# Name "sspi_protocol - Win32 Purify"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\scramble.c
# End Source File
# Begin Source File

SOURCE=.\sspi.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\protocol_interface.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

