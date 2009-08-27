# build debug version of ezgdi
#DEBUG = 1

# dynamic linking to freetype.dll
#USE_FTDLL = 0

# specify directories for
# use visual c++ x64 cross tools
#USE_VCX64CROSS = 0

# specify directories for EasyHook
#  easyhook.h
EASYHOOK_INCDIR = ..\easyhook\public 
#  x86\easyhook32.lib x64\easyhook64.lib
EASYHOOK_LIBDIR = ..\easyhook\release\x86 ..\easyhook\release\x64

# specify directories for FreeType
FREETYPE_INCDIR = ..\freetype\include
#  freetype32.lib freetype64.lib, see README
FREETYPE_LIBDIR = ..\freetype

# use detours in 32-bit version
#USE_DETOURS = 1

# specify directories for detours
#  detours.h
DETOURS_INCDIR = ..\detours\include 
#  detours.lib detoured.lib
DETOURS_LIBDIR = ..\detours\lib

# use intel compiler to compile
#USE_ICC = 1

# compiler options
# you may not want to modify following configs
!ifdef USE_ICC
!    ifdef X64
CC = xx
!    else
CC = yy
!    endif
!else # MSVC
!    ifdef X64
LIB = $(VCINSTALLDIR)\ATLMFC\LIB\amd64;$(VCINSTALLDIR)\LIB\amd64;$(WINDOWSSDKDIR)\LIB\x64
!        ifdef USE_VCX64CROSS
CC = "$(VCINSTALLDIR)\BIN\amd64\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\amd64\link.exe"
!        else
CC = "$(VCINSTALLDIR)\BIN\x86_amd64\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\x86_amd64\link.exe"
!        endif
!    else # X86
LIB = $(VCINSTALLDIR)\ATLMFC\LIB;$(VCINSTALLDIR)\LIB;$(WINDOWSSDKDIR)\LIB
CC = "$(VCINSTALLDIR)\BIN\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\link.exe"
!    endif
!endif
