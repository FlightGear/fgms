# Microsoft Developer Studio Project File - Name="libsgdebug_a" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libsgdebug_a - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsgdebug_a.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsgdebug_a.mak" CFG="libsgdebug_a - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsgdebug_a - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libsgdebug_a - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libsgdebug_a - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\libsgdebug_a"
# PROP Intermediate_Dir "Release\libsgdebug_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /MT /I "." /I "..\src" /I ".." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /D "UL_WIN32" /D "UL_MSVC" /D "NO_TRACKER_PORT" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H" /D "NOMINMAX" /D "FREEGLUT_STATIC" /D "FREEGLUT_LIB_PRAGMAS" /D "OPENALSDK" /D "_SCL_SECURE_NO_WARNINGS" /D "__CRT_NONSTDC_NO_WARNINGS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\libsgdebug_a.lib"

!ELSEIF  "$(CFG)" == "libsgdebug_a - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\libsgdebug_a"
# PROP Intermediate_Dir "Debug\libsgdebug_a"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /MTd /I "." /I "..\src" /I ".." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "_CRT_SECURE_NO_WARNINGS" /D "UL_WIN32" /D "UL_MSVC" /D "NO_TRACKER_PORT" /D "_USE_MATH_DEFINES" /D "_CRT_SECURE_NO_DEPRECATE" /D "HAVE_CONFIG_H" /D "NOMINMAX" /D "FREEGLUT_STATIC" /D "FREEGLUT_LIB_PRAGMAS" /D "OPENALSDK" /D "_SCL_SECURE_NO_WARNINGS" /D "__CRT_NONSTDC_NO_WARNINGS" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\libsgdebug_aD.lib"

!ENDIF 

# Begin Target

# Name - "libsgdebug_a - Win32 Release"
# Name - "libsgdebug_a - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=..\src\simgear\debug\logstream.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=..\src\simgear\debug\debug_types.h
# End Source File
# Begin Source File

SOURCE=..\src\simgear\debug\logstream.hxx
# End Source File
# End Group
# End Target
# End Project
