cmake_minimum_required (VERSION 2.4.7)
project(video_player)

include_directories(../include)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)

set(LIBS
	${LIBS}
	avformat
	avcodec
	avutil
	SDL
	openal
)

add_library(video_player SHARED
	video_player.cpp
	../common.cpp
	../font.cpp
)

target_link_libraries(video_player ${LIBS})
