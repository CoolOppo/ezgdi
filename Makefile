PREFIX = ezgdi
RELEASE = 1
BASEVER = gdi0850
VERSION = $(PREFIX)-r$(RELEASE)-$(BASEVER)

.PHONY: all clean

all:
	@$(MAKE) X86=1 do-all
	@$(MAKE) X64=1 do-all

clean:
	@$(MAKE) X86=1 do-clean
	@$(MAKE) X64=1 do-clean
	@-rmdir /s /q build >NUL 2>NUL

!if defined(X86) || defined(X64)

!ifdef X64
ARCH=x64
!else
ARCH=x86
!endif

SRCDIR = ezgdi
OBJS = hook.obj override.obj settings.obj cache.obj misc.obj expfunc.obj ft.obj fteng.obj ft2vert.obj gdidll.res
!ifdef X86
OBJS = $(OBJS) optimize\memcpy__amd.obj
!endif

TABASE = $(PREFIX)-$(ARCH)
TARGET = $(TABASE).dll
OBJDIR = build\objs-$(ARCH)
!ifdef X64
OBJS = build\objs-x64\$(OBJS: = build\objs-x64\) #-)
!else
OBJS = build\objs-x86\$(OBJS: = build\objs-x86\) #-)
!endif	

.PHONY: do-all do-init do-build do-clean do-cleanobj

do-all:
	@$(MAKE) do-init
	@$(MAKE) do-build

do-init: $(OBJDIR)
	@xcopy /m /s /q /y "$(SRCDIR)" "$(OBJDIR)"

$(OBJDIR):
	@mkdir $(OBJDIR)
	@xcopy /s /q /y "$(SRCDIR)" "$(OBJDIR)"

do-build: $(TARGET)

do-clean: do-cleanobj
	@-erase /f /q "$(TABASE).*" >NUL 2>NUL

do-cleanobj:
	@-erase /s /f /q "$(OBJDIR)" >NUL 2>NUL
	@-rmdir /s /q "$(OBJDIR)" >NUL 2>NUL

!include config.mak

INCDIR = $(SRCDIR) $(FREETYPE_INCDIR) $(EASYHOOK_INCDIR) $(DETOURS_INCDIR)
LIBDIR = $(FREETYPE_LIBDIR) $(EASYHOOK_LIBDIR)  $(DETOURS_LIBDIR)
INCLUDE = $(INCLUDE);$(INCDIR: =;)
LIB = $(LIB);$(LIBDIR: =;)
LIBPATH = $(LIB)

LIBS = advapi32.lib usp10.lib
!ifdef X64
LIBS = $(LIBS) freetype64.lib easyhook64.lib
!else # X86
!    ifdef USE_DETOURS
LIBS = $(LIBS) freetype32.lib detoured.lib detours.lib
!    else
LIBS = $(LIBS) freetype32.lib easyhook32.lib
!    endif
!endif

CFLAGS = /EHsc /GF /GL /Gy /MT /O2 /W3 
!ifdef X86
CFLAGS = /arch:SSE2 $(CFLAGS)
!endif

LDFLAGS = /nologo /opt:icf /opt:ref /ltcg $(LIBS)

$(TARGET): $(OBJS) $(SRCDIR)\expfunc.def
	$(LD) /dll $(LDFLAGS) /def:$(SRCDIR)\expfunc.def /out:$@ $(OBJS)

.SUFFIXES: .cpp .obj .rc .res

.cpp.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

.c.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

.rc.res:
	rc /l 0x411 $<

!endif # defined(ARCH)

