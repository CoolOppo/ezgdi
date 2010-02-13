#pragma once

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

LPWSTR _StrDupExAtoW(LPCSTR pszMB, int cchMB, LPWSTR pszStack, int cchStack, int* pcchWC);
static inline LPWSTR _StrDupAtoW(LPCSTR pszMB, int cchMB = -1, int* pcchWC = NULL)
{
   return _StrDupExAtoW(pszMB, cchMB, NULL, 0, pcchWC);
}

#define NOCOPY(T)            T(const T&); T& operator=(const T&)
#define countof(array)       (sizeof(array)/sizeof(array[0]))
#define sizeof_struct(s, m)  (((int)((char*)(&((s*)0)->m) - ((char*)((s*)0)))) + sizeof(((s*)0)->m))
#ifndef offsetof
#define offsetof(s,m)        (size_t)&(((s*)0)->m)
#endif

template<typename T> FORCEINLINE T Min(T x, T y) { return (x < y) ? x : y; }
template<typename T> FORCEINLINE T Max(T x, T y) { return (y < x) ? x : y; }
template<typename T> FORCEINLINE T Bound(T x, T m, T M) { return (x < m) ? m : ((x > M) ? M : x); }
template<typename T> FORCEINLINE int Sgn(T x, T y) { return (x > y) ? 1 : ((x < y) ? -1 : 0); }

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

