# Microsoft Developer Studio Project File - Name="raytracer7" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=raytracer7 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "raytracer7.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "raytracer7.mak" CFG="raytracer7 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "raytracer7 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "raytracer7 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=C:\Program Files\Codeplay\vectorcl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FAs /Fr /YX /FD -O3 -QaxW -Qipo -Qrcd -Qpc32 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /map /debug /machine:I386
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "raytracer7 - Win32 Release"
# Name "raytracer7 - Win32 Debug"
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\memory.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\memory.h
# End Source File
# Begin Source File

SOURCE=.\raytracer.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\raytracer.h
# End Source File
# Begin Source File

SOURCE=.\scene.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\scene.h
# End Source File
# Begin Source File

SOURCE=.\surface.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\surface.h
# End Source File
# Begin Source File

SOURCE=.\testapp.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\twister.cpp

!IF  "$(CFG)" == "raytracer7 - Win32 Release"

# ADD CPP /D "_CONFIGCP_Win32_20Release" /D "_PRJCP_raytracer7_2Evprj"

!ELSEIF  "$(CFG)" == "raytracer7 - Win32 Debug"

# ADD CPP /D "_CONFIGCP_Win32_20Debug" /D "_PRJCP_raytracer7_2Evprj"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\twister.h
# End Source File
# End Target
# End Project
