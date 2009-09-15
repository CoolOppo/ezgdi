PREFIX = freetype
VERSION = 2.3.9
MYMAKE = $(MAKE) -f freetype.mak

all:
	@$(MYMAKE) /nologo X86=1 do-all
	@$(MYMAKE) /nologo X64=1 do-all

clean:
	@$(MYMAKE) /nologo X86=1 do-clean
	@$(MYMAKE) /nologo X64=1 do-clean
	
!if defined(X86) || defined(X64)

!include config.mak

TABASE = $(PREFIX)-$(ARCH)
TARGET = $(FREETYPE_DIR)\$(TABASE).lib
OBJDIR = build\$(TABASE)
SRCDIR = $(FREETYPE_SRCDIR)

OBJS = $(OBJDIR)\autofit\autofit.obj $(OBJDIR)\bdf\bdf.obj $(OBJDIR)\cff\cff.obj $(OBJDIR)\base\ftbase.obj $(OBJDIR)\base\ftbitmap.obj $(OBJDIR)\cache\ftcache.obj $(OBJDIR)\base\ftdebug.obj $(OBJDIR)\base\ftfstype.obj $(OBJDIR)\base\ftgasp.obj $(OBJDIR)\base\ftglyph.obj $(OBJDIR)\gzip\ftgzip.obj $(OBJDIR)\base\ftinit.obj $(OBJDIR)\base\ftlcdfil.obj $(OBJDIR)\lzw\ftlzw.obj $(OBJDIR)\base\ftotval.obj $(OBJDIR)\base\ftstroke.obj $(OBJDIR)\base\ftsystem.obj $(OBJDIR)\smooth\smooth.obj $(OBJDIR)\base\ftbbox.obj $(OBJDIR)\base\ftmm.obj $(OBJDIR)\base\ftpfr.obj $(OBJDIR)\base\ftsynth.obj $(OBJDIR)\base\fttype1.obj $(OBJDIR)\base\ftwinfnt.obj $(OBJDIR)\pcf\pcf.obj $(OBJDIR)\pfr\pfr.obj $(OBJDIR)\psaux\psaux.obj $(OBJDIR)\pshinter\pshinter.obj $(OBJDIR)\psnames\psmodule.obj $(OBJDIR)\raster\raster.obj $(OBJDIR)\sfnt\sfnt.obj $(OBJDIR)\truetype\truetype.obj $(OBJDIR)\type1\type1.obj $(OBJDIR)\cid\type1cid.obj $(OBJDIR)\type42\type42.obj $(OBJDIR)\winfonts\winfnt.obj

do-all:
	@$(MYMAKE) /nologo do-init
	@$(MYMAKE) /nologo do-build

do-init: $(OBJDIR)
	@xcopy /t /q /y "$(SRCDIR)" "$(OBJDIR)"

$(OBJDIR):
	@mkdir $(OBJDIR)

do-build: $(TARGET)

do-clean: do-cleanobj
	@-erase /f /q "$(TABASE).*" >NUL 2>NUL

do-cleanobj:
	@-erase /s /f /q "$(OBJDIR)" >NUL 2>NUL
	@-rmdir /s /q "$(OBJDIR)" >NUL 2>NU

INCLUDE = $(INCLUDE);$(FREETYPE_INCDIR)
DEFS = /DNDEBUG /DWIN32 /D_LIB /D_CRT_SECURE_NO_WARNINGS /DFT2_BUILD_LIBRARY
CFLAGS = $(CFLAGS) $(DEFS)

$(TARGET): $(OBJS)
	$(LD) /lib /out:$@ $(OBJS)

{$(SRCDIR)\autofit}.c{$(OBJDIR)\autofit}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\bdf}.c{$(OBJDIR)\bdf}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\base}.c{$(OBJDIR)\base}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\cff}.c{$(OBJDIR)\cff}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\pcf}.c{$(OBJDIR)\pcf}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\raster}.c{$(OBJDIR)\raster}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\psnames}.c{$(OBJDIR)\psnames}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\psaux}.c{$(OBJDIR)\psaux}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\pshinter}.c{$(OBJDIR)\pshinter}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\sfnt}.c{$(OBJDIR)\sfnt}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\cid}.c{$(OBJDIR)\cid}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\type1}.c{$(OBJDIR)\type1}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\type42}.c{$(OBJDIR)\type42}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<
	
{$(SRCDIR)\smooth}.c{$(OBJDIR)\smooth}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\cache}.c{$(OBJDIR)\cache}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\gzip}.c{$(OBJDIR)\gzip}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\lzw}.c{$(OBJDIR)\lzw}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\pcf}.c{$(OBJDIR)\pcf}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\pfr}.c{$(OBJDIR)\pfr}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\truetype}.c{$(OBJDIR)\truetype}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\winfonts}.c{$(OBJDIR)\winfonts}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<

{$(SRCDIR)\autofit}.c{$(OBJDIR)\autofit}.obj:
	$(CC) /nologo $(CFLAGS) /Fo$@ /c $<
!endif
