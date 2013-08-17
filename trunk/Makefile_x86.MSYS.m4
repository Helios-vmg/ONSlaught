changequote([,])dnl
define(LIBS,[dnl
-lSDL_image dnl
-lbz2 dnl
-lFLAC_static dnl
-lfreetype dnl
-ljpeg dnl
-lmikmod dnl
-lmpg123 dnl
-lpng dnl
-lOpenAL32 dnl
-lSDL dnl
-lSDLmain dnl
-ltiff dnl
-ltimidity dnl
-lvorbisfile dnl
-lvorbis dnl
-logg dnl
-lz dnl
])dnl
CC = gcc
CXX = g++
NONSCXXFLAGS = -DWIN32 -DUNICODE -DWINVER=0x0500 ifdef([SVN],[-DNONS_SVN ])-DBUILD_ONSLAUGHT -w -g0 -O3 -Iinclude -Iinclude/mpg123 -Iinclude/mpg123/mingw -c
NONSCFLAGS = $(NONSCXXFLAGS)
PLUGINCXXFLAGS = -DWIN32 -DUNICODE -DWINVER=0x0500 ifdef([SVN],[-DNONS_SVN ])-DBUILD_PLUGIN -w -g0 -O3 -Iinclude
NONSLDFLAGS = -Llib-win32 -static -static-libgcc -static-libstdc++ -s -lmingw32 LIBS -mwindows -Wl,--out-implib=bin-win32/libONSlaught.a
PLUGINLDFLAGS = -Llib-win32 -Lbin-win32 -static -static-libgcc -static-libstdc++ -s -lSDL -lONSlaught -mwindows -shared -Wl,--dll
RC = windres
RCFLAGS = -J rc -O coff
INPUTS = syscmd([cat sources.lst | gawk '{ strings[0]; match($0,"(.*)/(.*)\\.(.*)",strings); printf("objs/%s.o ",strings[2]); }'])

all: bin-win32/ONSlaught.exe

clean:
	rm -rf objs

plugin: bin-win32/plugin.dll

objs:
	(mkdir -p bin-win32 || mkdir -p objs) && mkdir -p objs

bin-win32/ONSlaught.exe: objs $(INPUTS) objs/onslaught.res
	$(CXX) $(INPUTS) objs/onslaught.res $(NONSLDFLAGS) -o bin-win32/ONSlaught.exe

bin-win32/plugin.dll: bin-win32/libONSlaught.a
	$(CXX) $(PLUGINCXXFLAGS) src/Plugin/Plugin.cpp $(PLUGINLDFLAGS) -o bin-win32/plugin.dll

bin-win32/libONSlaught.a: bin-win32/ONSlaught.exe

objs/onslaught.res: onslaught.rc
	$(RC) $(RCFLAGS) $< -o $@
syscmd([cat sources.lst | gawk '{ strings[0]; match($0,"(.*)/(.*)\\.(.*)",strings); format=""; if (strings[3]=="cpp") format="objs/%s.o: %s/%s.%s\n\t$(CXX) $(NONSCXXFLAGS) $< -o $@\n"; else format="objs/%s.o: %s/%s.%s\n\t$(CC) $(NONSCFLAGS) $< -o $@\n"; printf(format,strings[2],strings[1],strings[2],strings[3]); }'])dnl
