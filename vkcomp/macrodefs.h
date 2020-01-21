#ifndef MACRO_DEFS_H
#define MACRO_DEFS_H

#define DEBUG_MODE

#ifdef _WIN32
	#define CLOCK_MONOTONIC 0xdeadbeef
#endif

#define DBG_PRINT(x) std::cout << x << std::endl; 

#define PPTR(x) (void**)&x

#define FATAL_EXIT(x) {\
std::cout << "FATAL " << x << std::endl; \
exit(-1);\
}

#endif
