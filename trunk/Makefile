PREFIX = ezgdi
RELEASE = 1
BASEVER = gdi0850
VERSION = $(PREFIX)-r$(RELEASE)-$(BASEVER)

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

TABASE = $(PREFIX)-$(ARCH)
TARGET = $(TABASE).dll
OBJDIR = build\objs-$(ARCH)
SRCDIR = ezgdi

OBJS = hook.obj override.obj settings.obj cache.obj misc.obj expfunc.obj ft.obj fteng.obj ft2vert.obj gdidll.res
!ifdef X86
OBJS = $(OBJS) memcpy__amd.obj
OBJS = build\objs-x86\$(OBJS: = build\objs-x86\) #-)
!else
OBJS = build\objs-x64\$(OBJS: = build\objs-x64\) #-)
!endif	

do-all:
	@$(MAKE) do-init
	@$(MAKE) do-build

do-init: $(OBJDIR)
	@xcopy /t /q /y "$(SRCDIR)" "$(OBJDIR)"

$(OBJDIR):
	@mkdir $(OBJDIR)

do-build: $(TARGET)
	echo build $*

do-clean: do-cleanobj
	echo clean
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

!if defined(X86) && defined(USE_DETOURS)
DEFS = $(DEFS) /DUSE_DETOURS
!endif

CFLAGS = /EHsc /GF /GL /Gy /MT /O2 /W3 $(DEFS)
!ifdef X86
CFLAGS = /arch:SSE2 $(CFLAGS)
!endif

LDFLAGS = /nologo /opt:icf /opt:ref /ltcg $(LIBS)

$(TARGET): $(OBJS) $(SRCDIR)\expfunc.def
	$(LD) /dll $(LDFLAGS) /def:$(SRCDIR)\expfunc.def /out:$@ $(OBJS)

.SUFFIXES: .cpp .obj .rc .res

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\optimize}.cpp{$(OBJDIR)}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)}.c{$(OBJDIR)}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)}.rc{$(OBJDIR)}.res:
	rc /fo $@ /l 0x411 $<

!endif # defined(ARCH)

