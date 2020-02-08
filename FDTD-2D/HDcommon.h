#ifndef _HD_COMMON_FDTD2D
#define _HD_COMMON_FDTD2D

#define DATA_TYPE float
#define tmax 500
#ifndef __ANDROID__
#define NX 2048
#define NY 2048
#else
#define NX 1024
#define NY 1024
#endif

#endif