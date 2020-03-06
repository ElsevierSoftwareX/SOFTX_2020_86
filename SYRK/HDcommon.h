#ifndef _HD_COMMON_SYRK
#define _HD_COMMON_SYRK

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define N 1024
#define M 1024
#else
#define N 512
#define M 512
#endif

#define alpha 12435
#define beta 4546

#endif