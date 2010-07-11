cmake_minimum_required (VERSION 2.4.7)
project(onslaught)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)
add_definitions(-DNONS_SVN)
add_definitions(-DHAVE_INTTYPES_H -DSDL_HAS_64BIT_TYPE -w)

set(LIBS
	${LIBS}
	SDL
	SDL_image
	SDL_mixer
	bz2
	freetype
	dl
	z
)

add_executable(ONSlaught
src/Archive.cpp
src/Audio.cpp
src/CommandLineOptions.cpp
src/ConfigFile.cpp
src/ErrorCodes.cpp
src/ExpressionParser.tab.cpp
src/FileLog.cpp
src/Functions.cpp
src/GFX.cpp
src/GUI.cpp
src/INIParser.tab.cpp
src/INIfile.cpp
src/IOFunctions.cpp
src/ImageLoader.cpp
src/LZMA.c
src/MacroParser.cpp
src/MacroParser.tab.cpp
src/ONSlaught.cpp
src/SDL_bilinear.c
src/SJIS.table.cpp
src/SaveFile.cpp
src/ScreenSpace.cpp
src/Script.cpp
src/ScriptInterpreter.cpp
src/ThreadManager.cpp
src/VariableStore.cpp
src/VirtualScreen.cpp
src/sha1.cpp
src/Plugin/LibraryLoader.cpp 
)

add_library(plugin SHARED
	src/Plugin/Plugin.cpp
)

target_link_libraries(ONSlaught ${LIBS})
