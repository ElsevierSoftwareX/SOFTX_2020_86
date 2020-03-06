#ifndef _HD_COMMON_3DCONV
#define _HD_COMMON_3DCONV

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define NI 256
#define NJ 256
#define NK 256
#else
#define NI 128
#define NJ 128
#define NK 128
#endif

#endif