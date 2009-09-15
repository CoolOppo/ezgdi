PREFIX = ezgdi
RELEASE = 1
BASEVER = gdi0850
VERSION = $(PREFIX)-r$(RELEASE)-$(BASEVER)

all:
	@$(MAKE) /nologo X86=1 do-all
	@$(MAKE) /nologo X64=1 do-all

clean:
	@$(MAKE) /nologo X86=1 do-clean
	@$(MAKE) /nologo X64=1 do-clean
	@-rmdir /s /q build >NUL 2>NUL

!if defined(X86) || defined(X64)

!include config.mak

TABASE = $(PREFIX)-$(ARCH)
TARGET = $(TABASE).dll
OBJDIR = build\objs-$(ARCH)
SRCDIR = ezgdi

OBJS = hook.obj override.obj settings.obj cache.obj misc.obj expfunc.obj ft.obj fteng.obj ft2vert.obj gdidll.res
!ifdef X86
OBJS = $(OBJS) optimize\memcpy__amd.obj
OBJS = build\objs-x86\$(OBJS: = build\objs-x86\) #-)
!else
OBJS = build\objs-x64\$(OBJS: = build\objs-x64\) #-)
!endif	

do-all:
	@$(MAKE) /nologo do-init
	@$(MAKE) /nologo do-build

do-init: $(OBJDIR)
	@xcopy /t /q /y "$(SRCDIR)" "$(OBJDIR)"

$(FREETYPE_DIR)\freetype-x86.lib: freetype.mak
	$(MAKE) -f freetype.mak X86=1

$(FREETYPE_DIR)\freetype-x64.lib: freetype.mak
	$(MAKE) -f freetype.mak X64=1

$(OBJDIR):
	@mkdir $(OBJDIR)

do-build: $(TARGET)

do-clean: do-cleanobj
	@-erase /f /q "$(TABASE).*" >NUL 2>NUL

do-cleanobj:
	@-erase /s /f /q "$(OBJDIR)" >NUL 2>NUL
	@-rmdir /s /q "$(OBJDIR)" >NUL 2>NUL

INCDIR = $(SRCDIR) $(FREETYPE_INCDIR) $(EASYHOOK_INCDIR) $(DETOURS_INCDIR)
LIBDIR = $(FREETYPE_LIBDIR) $(EASYHOOK_LIBDIR)  $(DETOURS_LIBDIR)
INCLUDE = $(INCLUDE);$(INCDIR: =;)
LIB = $(LIB);$(LIBDIR: =;)
LIBPATH = $(LIB)

LIBS = advapi32.lib usp10.lib freetype-$(ARCH).lib
!ifdef X64
LIBS = $(LIBS) easyhook64.lib
!else # X86
!    ifdef USE_DETOURS
LIBS = $(LIBS) detoured.lib detours.lib
!    else
LIBS = $(LIBS) easyhook32.lib
!    endif
!endif
DEFS = /DWIN32 /D_WINDOWS /D_UNICODE /DUNICODE
!if defined(X86) && defined(USE_DETOURS)
DEFS = $(DEFS) /DUSE_DETOURS
!endif
CFLAGS = $(CFLAGS) $(DEFS)

$(TARGET): $(OBJS) $(SRCDIR)\expfunc.def $(FREETYPE_DIR)\freetype-$(ARCH).lib
	$(LD) /dll $(LDFLAGS) /def:$(SRCDIR)\expfunc.def /out:$@ $(OBJS)

.SUFFIXES: .cpp .obj .rc .res

{$(SRCDIR)}.c{$(OBJDIR)}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\optimize}.cpp{$(OBJDIR)\optimize}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)}.rc{$(OBJDIR)}.res:
	rc /fo $@ /l 0x411 $<

!endif # defined(ARCH)

