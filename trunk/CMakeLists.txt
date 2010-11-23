cmake_minimum_required (VERSION 2.4.7)
project(onslaught)

include_directories(include)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)
add_definitions(-DNONS_SVN)
add_definitions(-DBUILD_ONSLAUGHT -DHAVE_INTTYPES_H -DSDL_HAS_64BIT_TYPE -w)

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
src/SJIS.table.cpp
src/ScriptInterpreter.cpp
src/GUI.cpp
src/ExpressionParser.tab.cpp
src/Image.cpp
src/MacroParser.tab.cpp
src/INIParser.tab.cpp
src/Archive.cpp
src/ScreenSpace.cpp
src/LZMA.c
src/VariableStore.cpp
src/Script.cpp
src/SaveFile.cpp
src/GFX.cpp
src/IOFunctions.cpp
src/VirtualScreen.cpp
src/Functions.cpp
src/CommandLineOptions.cpp
src/Audio.cpp
src/sha1.cpp
src/MacroParser.cpp
src/ONSlaught.cpp
src/ErrorCodes.cpp
src/Thread.cpp
src/ConfigFile.cpp
src/FileLog.cpp
src/INIfile.cpp
src/ThreadManager.cpp
src/OpenGL.cpp
src/Plugin/LibraryLoader.cpp 
)

add_library(plugin SHARED
	src/Plugin/Plugin.cpp
)

target_link_libraries(ONSlaught ${LIBS})
