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
FREETYPE_DIR = ..\freetype
#  freetype includes
FREETYPE_INCDIR = $(FREETYPE_DIR)\include
#  freetype32.lib freetype64.lib, see README
FREETYPE_LIBDIR = $(FREETYPE_DIR)
#  freetype src dir
FREETYPE_SRCDIR = $(FREETYPE_DIR)\src

# use detours in 32-bit version
#USE_DETOURS = 1

# specify directories for detours
#  detours.h
DETOURS_INCDIR = ..\detours\include 
#  detours.lib detoured.lib
DETOURS_LIBDIR = ..\detours\lib

# use intel compiler to compile
#USE_ICC = 1

# common config
# you may not want to modify following lines
!ifdef X64
ARCH=x64
!else
ARCH=x86
!endif

# compiler options
!ifdef X64
LIB = $(VCINSTALLDIR)\ATLMFC\LIB\amd64;$(VCINSTALLDIR)\LIB\amd64;$(WINDOWSSDKDIR)\LIB\x64
!else # X86
LIB = $(VCINSTALLDIR)\ATLMFC\LIB;$(VCINSTALLDIR)\LIB;$(WINDOWSSDKDIR)\LIB
!endif

!ifdef USE_ICC
!ifdef X64
LIB = $(ICPP_COMPILER11)\tbb\em64t\vc9\lib;$(ICPP_COMPILER11)\lib\intel64;$(LIB)
!else #X86
LIB = $(ICPP_COMPILER11)\tbb\ia32\vc9\lib;$(ICPP_COMPILER11)\lib\ia32;$(LIB)
!endif
!endif

LIBPATH = $(LIB)

!ifdef USE_ICC
LDFLAGS = /opt:icf /opt:ref $(LIBS)
CFLAGS = /GF /GS- /Gy /MT /O3 /QaxSSE2,SSE3,SSE3,SSE4.1 /Qipo /Qprec-div- /W3
!else
LDFLAGS = /opt:icf /opt:ref /ltcg $(LIBS)
CFLAGS = /GF /GL /GS- /Gy /MT /O2 /Oi /Ot /W3
!  ifdef X86
CFLAGS = /arch:SSE2 $(CFLAGS)
!  endif
!endif

!ifdef USE_ICC
!  ifdef X64
CC = "$(ICPP_COMPILER11)\bin\intel64\icl.exe"
LD = "$(ICPP_COMPILER11)\bin\intel64\xilink.exe"
!  else #X86
CC = "$(ICPP_COMPILER11)\bin\ia32\icl.exe"
LD = "$(ICPP_COMPILER11)\bin\ia32\xilink.exe"
!  endif
!else
!  ifndef X64
CC = "$(VCINSTALLDIR)\BIN\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\link.exe"
!  elseifdef USE_VCX64CROSS
CC = "$(VCINSTALLDIR)\BIN\x86_amd64\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\x86_amd64\link.exe"
!  else
CC = "$(VCINSTALLDIR)\BIN\amd64\cl.exe"
LD = "$(VCINSTALLDIR)\BIN\amd64\link.exe"
!  endif
!endif

