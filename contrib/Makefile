BIN=easyhook-localhook.exe rdtsc.exe apitracer.dll beep.exe msgbeep.dll

CPPFLAGS=/DUNICODE /D_UNICODE /DDEBUG /D_DEBUG /DEBUG /MTd /EHsc /W3 /I..\..\easyhook-2.6 /I..\..\detours\include
LINKFLAGS=/LIBPATH:..\..\easyhook-2.6 /LIBPATH:..\..\detours\lib detours.lib detoured.lib easyhook32.lib user32.lib advapi32.lib usp10.lib gdi32.lib /DEBUG

all: $(BIN)

.cpp.exe:
	$(CC) $(CPPFLAGS) $**  /link $(LINKFLAGS)
	mt.exe -manifest $@.manifest -outputresource:$@;1

.cpp.dll:
	$(CC) $(CPPFLAGS) $**  /link /out:$@ /dll $(LINKFLAGS)
	mt.exe -manifest $@.manifest -outputresource:$@;2

clean:
	-rm *.obj *.ilk *.pdb *.manifest $(BIN) >NUL 2>NUL
