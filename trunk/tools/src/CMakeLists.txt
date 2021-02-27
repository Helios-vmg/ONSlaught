cmake_minimum_required (VERSION 2.4.7)
project (nsaio)

set(EXECUTABLE_OUTPUT_PATH .)
set(CMAKE_BUILD_TYPE Release)

set(LIBS
	${LIBS}
	boost_filesystem
	boost_thread
	bz2
)

add_executable(nsaio
	nsaio.cpp
	Archive.cpp
	Unicode.cpp
	File.cpp
)

add_executable(zip
	zip.cpp
	LZMA.c
	Archive.cpp
	Unicode.cpp
	File.cpp
)

target_link_libraries(nsaio
	${LIBS}
)

target_link_libraries(zip
	${LIBS}
)
