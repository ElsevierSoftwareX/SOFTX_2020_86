#ifndef _HD_COMMON_2DCONV
#define _HD_COMMON_2DCONV

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define NI 4096
#define NJ 4096
#else
#define NI 2048
#define NJ 2048
#endif

#endif