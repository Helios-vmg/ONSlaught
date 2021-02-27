cmake_minimum_required (VERSION 2.4.7)
project(svg_loader)

include_directories(../include)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)

execute_process(
	COMMAND pkg-config --cflags QtCore QtGui QtSvg
	OUTPUT_VARIABLE QT_FLAGS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
	COMMAND pkg-config --libs QtCore QtGui QtSvg
	OUTPUT_VARIABLE QT_LIBS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS
	${CMAKE_CXX_FLAGS}
	${QT_FLAGS}
)

set(LIBS
	${LIBS}
	SDL
	${QT_LIBS}
)

add_library(svg_loader SHARED
	svg_loader.cpp
)

target_link_libraries(svg_loader ${LIBS})