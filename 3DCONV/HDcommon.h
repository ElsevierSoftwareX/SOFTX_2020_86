#ifndef _HD_COMMON_3DCONV
#define _HD_COMMON_3DCONV

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