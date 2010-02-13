# build debug version of ezgdi
#DEBUG = 1

# dynamic linking to freetype.dll
#USE_FTDLL = 0

# specify directories for
# use visual c++ x64 cross tools
USE_VCX64CROSS = 1

# link to easyhook debug lib for debug version
# USE_DEBUG_LIB = 1

# specify directories for EasyHook
#  easyhook.h
EASYHOOK_INCDIR = ..\easyhook-2.6
#  x86\easyhook32.lib x64\easyhook64.lib
EASYHOOK_LIBDIR = $(EASYHOOK_INCDIR)

# specify directories for FreeType
FREETYPE_DIR = ..\freetype-2.3.11
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

CFLAGS_DEBUG = /Od /MTd /FD /RTC1 /Zi /W4 /DDEBUG /D_DEBUG
LDFLAGS_DEBUG = /incremental:no /debug /opt:ref /opt:noicf /map /nodefaultlib:libcmt

!ifdef USE_ICC
LDFLAGS = /opt:icf /opt:ref
CFLAGS = /GS- /MT /O3 /QaxSSE2,SSE3,SSE3,SSE4.1 /Qipo /Qprec-div- /W4 /DNDEBUG
CFLAGS_SAFE = /GS- /MT /O1 /GF /Gs /Og /Os /Oi- /Gy /Ob2 /QaxSSE2,SSE3,SSE3,SSE4.1 /Qipo /Qprec-div- /W4 /DNDEBUG
!  ifdef X86
CFLAGS_SAFE = $(CFLAGS_SAFE) /Oy
!  endif
!else
LDFLAGS = /opt:icf /opt:ref /ltcg
CFLAGS_SAFE = /GF /GL /GS- /Gy /MT /O2 /Oi /Ot /W4 /DNDEBUG
!  ifdef X86
CFLAGS_SAFE = /arch:SSE2 $(CFLAGS_SAFE)
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

