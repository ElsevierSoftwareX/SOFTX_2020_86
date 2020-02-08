#ifndef _HD_COMMON_GEMM
#define _HD_COMMON_GEMM

#define DATA_TYPE float
#ifndef __ANDROID__
#define NI 512
#define NJ 512
#define NK 512
#else
#define NI 256
#define NJ 256
#define NK 256
#endif

#define ALPHA 32412.0f
#define BETA 2123.0f

#endif