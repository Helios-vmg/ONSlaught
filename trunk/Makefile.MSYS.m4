changequote([,])dnl
CXX = g++
NONSCXXFLAGS = -DWIN32 -DUNICODE -DWINVER=0x0500 ifdef([SVN],[-DNONS_SVN ])-DBUILD_ONSLAUGHT -W -O3 -Iinclude -c
PLUGINCXXFLAGS = -DWIN32 -DUNICODE -DWINVER=0x0500 ifdef([SVN],[-DNONS_SVN ])-DBUILD_PLUGIN -W -O3 -Iinclude
NONSLDFLAGS = -Llib-win32 -static-libgcc -s -lmingw32 -lSDL_mixer -ltimidity -lSDL_image -lsmpeg -lSDLmain -lSDL -lvorbisfile -lvorbis -ltiff -lpng -lmikmod -logg -ljpeg -lfreetype -lbz2 -lz -mwindows -Wl,--out-implib=bin-win32/libONSlaught.a
PLUGINLDFLAGS = -Llib-win32 -Lbin-win32 -static-libgcc -s -lSDL -lONSlaught -mwindows -shared -Wl,--dll
RC = windres
RCFLAGS = -J rc -O coff
INPUTS = syscmd([cat sources.lst | gawk '{ strings[0]; match($0,"(.*)/(.*)\\.(.*)",strings); printf("objs/%s.o ",strings[2]); }'])

all: objs bin-win32/ONSlaught.exe bin-win32/plugin.dll

clean:
	rm -rf objs

objs:
	(mkdir bin-win32 || mkdir objs) && mkdir objs

bin-win32/ONSlaught.exe: $(INPUTS) objs/onslaught.res
	$(CXX) $^ $(NONSLDFLAGS) -o bin-win32/ONSlaught.exe

bin-win32/plugin.dll: bin-win32/libONSlaught.a
	$(CXX) $(PLUGINCXXFLAGS) src/Plugin/Plugin.cpp $(PLUGINLDFLAGS) -o bin-win32/plugin.dll

bin-win32/libONSlaught.a: bin-win32/ONSlaught.exe

objs/onslaught.res: onslaught.rc
	$(RC) $(RCFLAGS) $< -o $@
syscmd([cat sources.lst | gawk '{ strings[0]; match($0,"(.*)/(.*)\\.(.*)",strings); printf("objs/%s.o: %s/%s.%s\n\t$(CXX) $(NONSCXXFLAGS) $< -o $@\n",strings[2],strings[1],strings[2],strings[3]); }'])dnl
