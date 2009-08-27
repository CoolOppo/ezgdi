#include "memcpy_amd.h"

#undef ZeroMemory
#undef memzero
#undef memset
#undef memcpy

#define ZeroMemory memzero_optimized
#define memzero memzero_optimized
#define memset memset_optimized
#define memcpy memcpy_optimized
