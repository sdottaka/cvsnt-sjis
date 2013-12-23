# Microsoft Developer Studio Project File - Name="cvsntcpl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=cvsntcpl - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "cvsntcpl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "cvsntcpl.mak" CFG="cvsntcpl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "cvsntcpl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cvsntcpl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "cvsntcpl - Win32 Purify" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "cvsnt"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "cvsntcpl - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32;NDEBUG;_WINDOWS;_USRDLL" /D "_UNICODE" /D "_AFXDLL" /D "_UNICODE" /GF /GR /Yu"stdafx.h" /Fp".\Release/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MD /Zi /W3 /O2 /Ob2 /Oi /Op /Oy /G6 /GA /D "WIN32;NDEBUG;_WINDOWS;_USRDLL" /D "_UNICODE" /D "_AFXDLL" /D "_UNICODE" /GF /GR /Yu"stdafx.h" /Fp".\Release/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\cvsntcpl.tlb" /win32 
# ADD MTL /nologo /D"NDEBUG" /mktyplib203 /tlb".\Release\cvsntcpl.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_AFXDLL" /d "NDEBUG" 
# ADD RSC /l 2057 /d "_AFXDLL" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinRel\cvsnt.cpl" /version:4.0 /incremental:no /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release/cvsnt.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinRel\cvsnt.cpl" /version:4.0 /incremental:no /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /opt:ref /opt:icf /implib:".\Release/cvsnt.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "cvsntcpl - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32;_DEBUG;_WINDOWS;_USRDLL" /D "_UNICODE" /D "_AFXDLL" /D "_UNICODE" /Gm /Gy /GR /Yu"stdafx.h" /Fp".\Debug/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD CPP /nologo /MDd /ZI /W3 /Od /G6 /GA /D "WIN32;_DEBUG;_WINDOWS;_USRDLL" /D "_UNICODE" /D "_AFXDLL" /D "_UNICODE" /Gm /Gy /GR /Yu"stdafx.h" /Fp".\Debug/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /GZ /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\cvsntcpl.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\cvsntcpl.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_AFXDLL" /d "_DEBUG" 
# ADD RSC /l 2057 /d "_AFXDLL" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinDebug\cvsnt.cpl" /version:4.0 /incremental:yes /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/cvsnt.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinDebug\cvsnt.cpl" /version:4.0 /incremental:yes /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/cvsnt.lib" /MACHINE:I386

!ELSEIF  "$(CFG)" == "cvsntcpl - Win32 Purify"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Purify"
# PROP BASE Intermediate_Dir "Purify"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Purify"
# PROP Intermediate_Dir "Purify"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_AFXDLL" /D "_MBCS" /Gm /Gy /GR /Yu"stdafx.h" /Fp".\Debug/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD CPP /nologo /MDd /Zi /W3 /Od /G6 /GA /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_AFXDLL" /D "_MBCS" /Gm /Gy /GR /Yu"stdafx.h" /Fp".\Debug/cvsntcpl.pch" /Fo"$(IntDir)/" /Fd"$(IntDir)/" /c /GX 
# ADD BASE MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\cvsntcpl.tlb" /win32 
# ADD MTL /nologo /D"_DEBUG" /mktyplib203 /tlb".\Debug\cvsntcpl.tlb" /win32 
# ADD BASE RSC /l 2057 /d "_AFXDLL" /d "_DEBUG" 
# ADD RSC /l 2057 /d "_AFXDLL" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo 
# ADD BSC32 /nologo 
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinDebug\cvsnt.cpl" /version:4.0 /incremental:yes /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/cvsnt.lib" /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /out:"..\WinDebug\cvsnt.cpl" /version:4.0 /incremental:yes /def:".\cvsnt.def" /debug /pdb:"$(TargetDir)\$(TargetName).pdb" /pdbtype:sept /subsystem:windows /implib:".\Debug/cvsnt.lib" /MACHINE:I386

!ENDIF

# Begin Target

# Name "cvsntcpl - Win32 Release"
# Name "cvsntcpl - Win32 Debug"
# Name "cvsntcpl - Win32 Purify"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AdvancedPage.cpp
# End Source File
# Begin Source File

SOURCE=.\Applet.cpp
# End Source File
# Begin Source File

SOURCE=.\cvsnt.cpp
# End Source File
# Begin Source File

SOURCE=.\cvsnt.def
# End Source File
# Begin Source File

SOURCE=.\cvsnt1.cpp
# End Source File
# Begin Source File

SOURCE=.\NewRootDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\RepositoryPage.cpp
# End Source File
# Begin Source File

SOURCE=.\serverPage.cpp
# End Source File
# Begin Source File

SOURCE=SslSettingPage.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

!IF  "$(CFG)" == "cvsntcpl - Win32 Release"

# ADD CPP /nologo /Yc"stdafx.h" /GX 
!ELSEIF  "$(CFG)" == "cvsntcpl - Win32 Debug"

# ADD CPP /nologo /Yc"stdafx.h" /GZ /GX 
!ELSEIF  "$(CFG)" == "cvsntcpl - Win32 Purify"

# ADD CPP /nologo /Yc"stdafx.h" /GX 
!ENDIF

# End Source File
# Begin Source File

SOURCE=.\TooltipPropertyPage.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AdvancedPage.h
# End Source File
# Begin Source File

SOURCE=.\cvsnt.h
# End Source File
# Begin Source File

SOURCE=.\cvsnt1.h
# End Source File
# Begin Source File

SOURCE=.\NewRootDialog.h
# End Source File
# Begin Source File

SOURCE=.\RepositoryPage.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\serverPage.h
# End Source File
# Begin Source File

SOURCE=SslSettingPage.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TooltipPropertyPage.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\cvsntcpl.rc
# End Source File
# Begin Source File

SOURCE=.\res\cvsntcpl.rc2
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# End Group
# End Target
# End Project

