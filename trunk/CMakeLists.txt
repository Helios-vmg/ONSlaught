cmake_minimum_required (VERSION 2.4.7)
project(onslaught)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)
add_definitions(-DNONS_SVN)
add_definitions(-DBUILD_ONSLAUGHT -DHAVE_INTTYPES_H -DSDL_HAS_64BIT_TYPE -w -fno-exceptions)

set(LIBS
	${LIBS}
	SDL_image
	SDL
	bz2
	freetype
	FLAC
	vorbisfile
	vorbis
	ogg
	jpeg
	png
	tiff
	mikmod
	openal
	mpg123
	dl
	z
)

add_executable(ONSlaught
src/Archive.cpp
src/Audio.cpp
src/AudioFormats.cpp
src/CommandLineOptions.cpp
src/ErrorCodes.cpp
src/ExpressionParser.tab.cpp
src/FileLog.cpp
src/Functions.cpp
src/GFX.cpp
src/GUI.cpp
src/Image.cpp
src/INIfile.cpp
src/INIParser.tab.cpp
src/IOFunctions.cpp
src/libFLAC++/metadata.cpp
src/libFLAC++/stream_decoder.cpp
src/libFLAC++/stream_encoder.cpp
src/libtimidity/common.c
src/libtimidity/instrum.c
src/libtimidity/instrum_dls.c
src/libtimidity/mix.c
src/libtimidity/output.c
src/libtimidity/playmidi.c
src/libtimidity/readmidi.c
src/libtimidity/resample.c
src/libtimidity/stream.c
src/libtimidity/tables.c
src/libtimidity/timidity.c
src/LZMA.c
src/MacroParser.cpp
src/MacroParser.tab.cpp
src/ONSlaught.cpp
src/OpenAL.cpp
src/Plugin/LibraryLoader.cpp
src/ScreenSpace.cpp
src/Script.cpp
src/ScriptInterpreter.cpp
src/sha1.cpp
src/SJIS.table.cpp
src/Thread.cpp
src/ThreadManager.cpp
src/tinyxml/tinyxml.cpp
src/tinyxml/tinyxmlerror.cpp
src/tinyxml/tinyxmlparser.cpp
src/VariableStore.cpp
src/VirtualScreen.cpp
)

add_library(plugin SHARED
	src/Plugin/Plugin.cpp
)

target_link_libraries(ONSlaught ${LIBS})
