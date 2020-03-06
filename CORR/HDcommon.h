#ifndef _HD_COMMON_CORR
#define _HD_COMMON_CORR

/*
See LICENSE.md for vkpolybench and other 3rd party licenses. 
*/

#define DATA_TYPE float

#ifndef __ANDROID__
#define M 2048
#define N 2048
#else
#define M 512
#define N 512
#endif

#define FLOAT_N 3214212.01f
#define EPS 0.005f

#endif