#ifndef MACRO_DEFS_H
#define MACRO_DEFS_H

#define DEBUG_MODE

#ifdef _WIN32
	#define CLOCK_MONOTONIC 0xdeadbeef
#endif

#ifndef __ANDROID__
#define DBG_PRINT(x) std::cout << x << std::endl; 
#define DBG_PRINT2(x,y) std::cout << x << y << std::endl;
#else
#include <android/log.h>
#define DBG_PRINT(x) __android_log_print(ANDROID_LOG_INFO, "vulkanandroid", "%s", x);
#define DBG_PRINT2(x,y) __android_log_print(ANDROID_LOG_INFO, "vulkanandroid", "%s %s", x,y);
#endif

#define PPTR(x) (void**)&x

#ifndef __ANDROID__
#define FATAL_EXIT(x) {\
std::cout << "FATAL " << x << std::endl; \
exit(-1);\
}
#else
#define FATAL_EXIT(x) {\
__android_log_print(ANDROID_LOG_ERROR, "vulkanandroid", "%s", x);\
exit(-1);\
}
#endif


#endif
