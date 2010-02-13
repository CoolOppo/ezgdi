#pragma once

/* windows version defines */
#include "targetver.h"

/* wind32 headers */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <usp10.h>
#include <tlhelp32.h>

/* string processing headers */
#include <tchar.h>
#include <stddef.h>
#include <mbctype.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_GDI
#include <shlwapi.h>

/* std c headers */
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <malloc.h>
#include <crtdbg.h>

/* std c++ headers */
#include <functional>
#include <algorithm>

/* atl mfc headers */
#include <atlbase.h>

/* freetype headers */
#include <freetype/config/ftheader.h>
#include <freetype/freetype.h>
#include <freetype/ftcache.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <freetype/ftlcdfil.h>

/* hook api headers */
#ifdef USE_DETOURS
#include <detours.h>
#else
#include <easyhook.h>
#endif

/* project headers */
#include "optimize/optimize.h"
#include "array.h"
#include "common.h"
#include "cache.h"
#include "tlsdata.h"
