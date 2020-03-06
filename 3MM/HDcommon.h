#ifndef _HD_COMMON_3MM
#define _HD_COMMON_3MM

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float
#ifndef __ANDROID__
#define NI 512
#define NJ 512
#define NK 512
#define NL 512
#define NM 512
#else 
#define NI 256
#define NJ 256
#define NK 256
#define NL 256
#define NM 256
#endif

#endif