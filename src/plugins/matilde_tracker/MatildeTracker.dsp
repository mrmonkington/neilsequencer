# Microsoft Developer Studio Project File - Name="MatildeTracker" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=MatildeTracker - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "MatildeTracker.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "MatildeTracker.mak" CFG="MatildeTracker - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "MatildeTracker - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MatildeTracker - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MatildeTracker - Win32 Release Mono" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "MatildeTracker - Win32 Debug Mono" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/MatildeTracker", CEAAAAAA"
# PROP Scc_LocalPath "d:\code\matildetracker"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "MatildeTracker - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MD /W3 /O2 /I "g:\audio\buzz\dev" /I "Surf's DSP Lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BUZZ" /FAs /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x406 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"g:\audio\buzz\gear\generators\Matilde Tracker.dll"

!ELSEIF  "$(CFG)" == "MatildeTracker - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MDd /W3 /GX /ZI /Od /I "Surf's DSP Lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x406 /d "_DEBUG"
# ADD RSC /l 0x406 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"E:\program\buzz\gear\generators\Turtle Tracker.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "MatildeTracker - Win32 Release Mono"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MatildeTracker___Win32_Release_Mono"
# PROP BASE Intermediate_Dir "MatildeTracker___Win32_Release_Mono"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MatildeTracker___Win32_Release_Mono"
# PROP Intermediate_Dir "MatildeTracker___Win32_Release_Mono"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gr /Zp4 /MD /W3 /O2 /I "d:\audio\buzz\dev" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MD /W3 /O2 /I "g:\audio\buzz\dev" /I "Surf's DSP Lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MONO" /D "BUZZ" /FAs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib d:\audio\buzz\dev\dsplib\dsplib.lib /nologo /subsystem:windows /dll /machine:I386 /out:"d:\audio\buzz\gear\generators\Matilde Tracker.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"g:\audio\buzz\gear\generators\Matilde Tracker (Mono).dll"

!ELSEIF  "$(CFG)" == "MatildeTracker - Win32 Debug Mono"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MatildeTracker___Win32_Debug_Mono"
# PROP BASE Intermediate_Dir "MatildeTracker___Win32_Debug_Mono"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "MatildeTracker___Win32_Debug_Mono"
# PROP Intermediate_Dir "MatildeTracker___Win32_Debug_Mono"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Gr /Zp4 /MD /W3 /O2 /I "c:\audio\buzz\dev" /I "Surf's DSP Lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MONO" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /Zp4 /MD /W3 /Gi /ZI /Od /I "c:\audio\buzz\dev" /I "Surf's DSP Lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "MONO" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"c:\audio\buzz\gear\generators\Matilde Tracker (Mono).dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"E:\program\buzz\gear\generators\Turtle Tracker.dll"

!ENDIF 

# Begin Target

# Name "MatildeTracker - Win32 Release"
# Name "MatildeTracker - Win32 Debug"
# Name "MatildeTracker - Win32 Release Mono"
# Name "MatildeTracker - Win32 Debug Mono"
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Matilde Tracker.html"
# End Source File
# End Group
# Begin Group "Surf's DSP Lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_2PFilter.cpp"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_2PFilter.h"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_Amp.cpp"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_Amp.h"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_DSP.cpp"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_DSP.h"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_Resampler.cpp"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_Resampler.h"
# End Source File
# Begin Source File

SOURCE=".\Surf's DSP Lib\SRF_Types.h"
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Matilde.bmp
# End Source File
# Begin Source File

SOURCE=.\Script1.rc
# End Source File
# End Group
# Begin Source File

SOURCE=.\BuzzInstrument.cpp
# End Source File
# Begin Source File

SOURCE=.\BuzzInstrument.h
# End Source File
# Begin Source File

SOURCE=.\BuzzSample.cpp
# End Source File
# Begin Source File

SOURCE=.\BuzzSample.h
# End Source File
# Begin Source File

SOURCE=.\Channel.cpp
# End Source File
# Begin Source File

SOURCE=.\Channel.h
# End Source File
# Begin Source File

SOURCE=.\Envelope.cpp
# End Source File
# Begin Source File

SOURCE=.\Envelope.h
# End Source File
# Begin Source File

SOURCE=.\IInstrument.cpp
# End Source File
# Begin Source File

SOURCE=.\IInstrument.h
# End Source File
# Begin Source File

SOURCE=.\ISample.cpp
# End Source File
# Begin Source File

SOURCE=.\ISample.h
# End Source File
# Begin Source File

SOURCE=.\Track.cpp
# End Source File
# Begin Source File

SOURCE=.\Track.h
# End Source File
# Begin Source File

SOURCE=.\Tracker.cpp
# End Source File
# Begin Source File

SOURCE=.\Tracker.h
# End Source File
# Begin Source File

SOURCE=.\WavetableManager.cpp
# End Source File
# Begin Source File

SOURCE=.\WavetableManager.h
# End Source File
# End Target
# End Project
