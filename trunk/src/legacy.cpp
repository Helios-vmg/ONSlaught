#include <cstdio>

static FILE ios[] = { *stdin, *stdout, *stderr };

extern "C" FILE *__iob_func(){
	return ios;
}
