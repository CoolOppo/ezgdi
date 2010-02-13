#pragma once

#define Assert       _ASSERTE

class CCriticalSectionLock
{
private:
   static CRITICAL_SECTION m_cs;
   friend class CCriticalSectionLockTry;

public:
   CCriticalSectionLock()
   {
      ::EnterCriticalSection(&m_cs);
   }
   ~CCriticalSectionLock()
   {
      ::LeaveCriticalSection(&m_cs);
   }
   static void Init()
   {
      ::InitializeCriticalSection(&m_cs);
   }
   static void Term()
   {
      ::DeleteCriticalSection(&m_cs);
   }
};

class CCriticalSectionLockTry
{
public:
   BOOL TryEnter()
   {
      return ::TryEnterCriticalSection(&CCriticalSectionLock::m_cs);
   }
   void Leave()
   {
      ::LeaveCriticalSection(&CCriticalSectionLock::m_cs);
   }
};


FORCEINLINE HINSTANCE GetDLLInstance()
{
   extern HINSTANCE g_hinstDLL;
   return g_hinstDLL;
}

// Žg—pŒã‚Ífree‚ÅŠJ•ú‚·‚éŽ–
LPWSTR _StrDupExAtoW(LPCSTR pszMB, int cchMB, LPWSTR pszStack, int cchStack, int* pcchWC);
static inline LPWSTR _StrDupAtoW(LPCSTR pszMB, int cchMB = -1, int* pcchWC = NULL)
{
   return _StrDupExAtoW(pszMB, cchMB, NULL, 0, pcchWC);
}

// useful macros
#define NOCOPY(T)            T(const T&); T& operator=(const T&)
#define countof(array)       (sizeof(array)/sizeof(array[0]))
#define sizeof_struct(s, m)  (((int)((char*)(&((s*)0)->m) - ((char*)((s*)0)))) + sizeof(((s*)0)->m))
#ifndef offsetof
#define offsetof(s,m)        (size_t)&(((s*)0)->m)
#endif
#ifdef _DEBUG
#define Verify(expr)         _ASSERTE(expr)
#else
#define Verify(expr)         (expr)
#endif

template<typename T> FORCEINLINE T Min(T x, T y) { return (x < y) ? x : y; }
template<typename T> FORCEINLINE T Max(T x, T y) { return (y < x) ? x : y; }
template<typename T> FORCEINLINE T Bound(T x, T m, T M) { return (x < m) ? m : ((x > M) ? M : x); }
template<typename T> FORCEINLINE int Sgn(T x, T y) { return (x > y) ? 1 : ((x < y) ? -1 : 0); }

#ifdef _DEBUG
#undef DeletePen
#undef DeleteBrush
#undef DeleteRgn
#undef DeleteFont
#undef DeleteBitmap
#undef SelectPen
#undef SelectBrush
#undef SelectRgn
#undef SelectFont
#undef SelectBitmap

#define _IsValidPen(hPen)     \
   (hPen == NULL || ::GetObjectType(hPen) == OBJ_PEN || ::GetObjectType(hPen) == OBJ_EXTPEN)
#define _IsValidBrush(hBrush) \
   (hBrush == NULL || ::GetObjectType(hBrush) == OBJ_BRUSH)
#define _IsValidRgn(hRgn)     \
   (hRgn == NULL || ::GetObjectType(hRgn) == OBJ_REGION)
#define _IsValidFont(hFont)      \
   (hFont == NULL || ::GetObjectType(hFont) == OBJ_FONT)
#define _IsValidBitmap(hBitmap)  \
   (hBitmap == NULL || ::GetObjectType(hBitmap) == OBJ_BITMAP)

#define DEFINE_DELETE_FUNCTION(type, name) \
   FORCEINLINE BOOL WINAPI Delete##name(type h##name) \
   { \
      _ASSERTE(_IsValid##name(h##name)); \
      return ::DeleteObject(h##name); \
   }

#define DEFINE_SELECT_FUNCTION(type, name) \
   FORCEINLINE type WINAPI Select##name(HDC hDC, type h##name) \
   { \
      _ASSERTE(hDC != NULL); \
      _ASSERTE(_IsValid##name(h##name)); \
      return (type)::SelectObject(hDC, h##name); \
   }

DEFINE_DELETE_FUNCTION(HPEN,  Pen)
DEFINE_DELETE_FUNCTION(HBRUSH,   Brush)
DEFINE_DELETE_FUNCTION(HRGN,  Rgn)
DEFINE_DELETE_FUNCTION(HFONT, Font)
DEFINE_DELETE_FUNCTION(HBITMAP,  Bitmap)

DEFINE_SELECT_FUNCTION(HPEN,  Pen)
DEFINE_SELECT_FUNCTION(HBRUSH,   Brush)
DEFINE_SELECT_FUNCTION(HRGN,  Rgn)
DEFINE_SELECT_FUNCTION(HFONT, Font)
DEFINE_SELECT_FUNCTION(HBITMAP,  Bitmap)

#undef _IsValidPen
#undef _IsValidBrush
#undef _IsValidRgn
#undef _IsValidFont
#undef _IsValidBitmap
#undef DEFINE_DELETE_FUNCTION
#undef DEFINE_SELECT_FUNCTION

#pragma deprecated(DeleteObject)
#pragma deprecated(SelectObject)

#else //!_DEBUG
#define DeletePen       ::DeleteObject
#define DeleteBrush     ::DeleteObject
#define DeleteRgn       ::DeleteObject
#define DeleteFont      ::DeleteObject
#define DeleteBitmap    ::DeleteObject

#define SelectPen(d,o)     (HPEN)::SelectObject(d,o)
#define SelectBrush(d,o)   (HBRUSH)::SelectObject(d,o)
#define SelectRgn(d,o)     (HRGN)::SelectObject(d,o)
#define SelectFont(d,o)    (HFONT)::SelectObject(d,o)
#define SelectBitmap(d,o)  (HBITMAP)::SelectObject(d,o)
#endif //_DEBUG

//#define TRACE  NOP_FUNCTION
