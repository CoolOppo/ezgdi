#if 0
#include "memcpy_amd.h"

#ifdef _M_IX86
#undef ZeroMemory
#undef memzero
#undef memset
#undef memcpy

#define ZeroMemory memzero_optimized
#define memzero memzero_optimized
#define memset memset_optimized
#define memcpy memcpy_optimized
#endif
#endif