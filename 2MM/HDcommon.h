#ifndef _HD_COMMON_2MM
#define _HD_COMMON_2MM

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define NI 2048
#define NJ 2048
#define NK 2048
#define NL 2048
#else
#define NI 512
#define NJ 512
#define NK 512
#define NL 512
#endif

#endif