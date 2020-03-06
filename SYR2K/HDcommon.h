#ifndef _HD_COMMON_SYR2K
#define _HD_COMMON_SYR2K

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define N 2048
#define M 2048
#else
#define N 512
#define M 512
#endif

#define ALPHA 12435
#define BETA 4546

#endif