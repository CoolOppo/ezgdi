#pragma once

#define _CRT_SECURE_NO_DEPRECATE 1
#define WINVER 0x500		//win2k
#define _WIN32_WINNT 0x500	//win2k
#define WIN32_LEAN_AND_MEAN 1
#define UNICODE  1
#define _UNICODE 1

#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>
#include <usp10.h>
//#include <limits>
#include <functional>
//#include <iterator>
#include <algorithm>
#include "array.h"

#define for if(0);else for

#include <tchar.h>
#include <stddef.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <malloc.h>
#include <crtdbg.h>

#include "optimize/optimize.h"

#define ASSERT			_ASSERTE
#define Assert			_ASSERTE
#ifdef _DEBUG
#define new				new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef NOP_FUNCTION
#if (_MSC_VER >= 1210)
#define NOP_FUNCTION	__noop
#else
#define NOP_FUNCTION	(void)0
#endif	//_MSC_VER
#endif	//!NOP_FUNCTION
#ifndef C_ASSERT
#define C_ASSERT(e)		typedef char __C_ASSERT__[(e)?1:-1]
#endif	//!C_ASSERT
#ifndef FORCEINLINE
#if (_MSC_VER >= 1200)
#define FORCEINLINE		__forceinline
#else
#define FORCEINLINE		__inline
#endif	//_MSC_VER
#endif	//!FORCEINLINE


//排他制御
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

// 使用後はfreeで開放する事
LPWSTR _StrDupExAtoW(LPCSTR pszMB, int cchMB, LPWSTR pszStack, int cchStack, int* pcchWC);
static inline LPWSTR _StrDupAtoW(LPCSTR pszMB, int cchMB = -1, int* pcchWC = NULL)
{
	return _StrDupExAtoW(pszMB, cchMB, NULL, 0, pcchWC);
}

// useful macros
#define NOCOPY(T)					T(const T&); T& operator=(const T&)
#define countof(array)				(sizeof(array)/sizeof(array[0]))
#define sizeof_struct(s, m)			(((int)((char*)(&((s*)0)->m) - ((char*)((s*)0)))) + sizeof(((s*)0)->m))
#ifndef offsetof
#define offsetof(s,m)				(size_t)&(((s*)0)->m)
#endif
#ifdef _DEBUG
#define Verify(expr)				_ASSERTE(expr)
#else
#define Verify(expr)				(expr)
#endif

template<typename T> FORCEINLINE T Min(T x, T y) { return (x < y) ? x : y; }
template<typename T> FORCEINLINE T Max(T x, T y) { return (y < x) ? x : y; }
template<typename T> FORCEINLINE T Bound(T x, T m, T M) { return (x < m) ? m : ((x > M) ? M : x); }
template<typename T> FORCEINLINE int Sgn(T x, T y) { return (x > y) ? 1 : ((x < y) ? -1 : 0); }


//型チェック機能つきDeleteXXX/SelectXXX
//SelectObject/DeleteObjectは使用できなくなる

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

#define _IsValidPen(hPen)		\
	(hPen == NULL || ::GetObjectType(hPen) == OBJ_PEN || ::GetObjectType(hPen) == OBJ_EXTPEN)
#define _IsValidBrush(hBrush)	\
	(hBrush == NULL || ::GetObjectType(hBrush) == OBJ_BRUSH)
#define _IsValidRgn(hRgn)		\
	(hRgn == NULL || ::GetObjectType(hRgn) == OBJ_REGION)
#define _IsValidFont(hFont)		\
	(hFont == NULL || ::GetObjectType(hFont) == OBJ_FONT)
#define _IsValidBitmap(hBitmap)	\
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

DEFINE_DELETE_FUNCTION(HPEN,	Pen)
DEFINE_DELETE_FUNCTION(HBRUSH,	Brush)
DEFINE_DELETE_FUNCTION(HRGN,	Rgn)
DEFINE_DELETE_FUNCTION(HFONT,	Font)
DEFINE_DELETE_FUNCTION(HBITMAP,	Bitmap)

DEFINE_SELECT_FUNCTION(HPEN,	Pen)
DEFINE_SELECT_FUNCTION(HBRUSH,	Brush)
DEFINE_SELECT_FUNCTION(HRGN,	Rgn)
DEFINE_SELECT_FUNCTION(HFONT,	Font)
DEFINE_SELECT_FUNCTION(HBITMAP,	Bitmap)

#undef _IsValidPen
#undef _IsValidBrush
#undef _IsValidRgn
#undef _IsValidFont
#undef _IsValidBitmap
#undef DEFINE_DELETE_FUNCTION
#undef DEFINE_SELECT_FUNCTION

#if (_MSC_VER >= 1300)
#pragma deprecated(DeleteObject)
#pragma deprecated(SelectObject)
#else	//_MSC_VER < 1300
#undef DeleteObject
#define DeleteObject		DeleteObject_instead_use_DeleteXXX
#undef SelectObject
#define SelectObject		SelectObject_instead_use_DeleteXXX
#endif	//_MSC_VER

#else	//!_DEBUG
#ifndef _INC_WINDOWSX
#define DeletePen			::DeleteObject
#define DeleteBrush			::DeleteObject
#define DeleteRgn			::DeleteObject
#define DeleteFont			::DeleteObject
#define DeleteBitmap		::DeleteObject

#define SelectPen(d,o)		(HPEN)::SelectObject(d,o)
#define SelectBrush(d,o)	(HBRUSH)::SelectObject(d,o)
#define SelectRgn(d,o)		(HRGN)::SelectObject(d,o)
#define SelectFont(d,o)		(HFONT)::SelectObject(d,o)
#define SelectBitmap(d,o)	(HBITMAP)::SelectObject(d,o)
#endif	//!_INC_WINDOWSX
#endif	//_DEBUG


//TRACEマクロ
//使用例: TRACE(_T("cx: %d\n"), cx);
#ifdef _DEBUG
#define TRACE	_Trace
#include <stdarg.h>	//va_list
#include <stdio.h>	//_vsnwprintf

static void _Trace(LPCTSTR pszFormat, ...)
{
	CCriticalSectionLock __lock;
	va_list argptr;
	va_start(argptr, pszFormat);
	//w(v)sprintfは1024文字以上返してこない
	TCHAR szBuffer[1024];
	wvsprintf(szBuffer, pszFormat, argptr);

	//デバッガをアタッチしてる時はデバッガにメッセージを出す
	if (IsDebuggerPresent()) {
		OutputDebugString(szBuffer);
		return;
	}

	extern HANDLE g_hfDbgText;
	HANDLE hf = g_hfDbgText;
	if (!hf) {
		TCHAR szFileName[MAX_PATH+16];
		GetModuleFileName(GetDLLInstance(), szFileName, MAX_PATH);
		TCHAR *p1 = _tcsrchr(szFileName, _T('\\'));
		if (p1 && p1 > szFileName) {
			*p1 = 0;
			TCHAR *p2 = _tcsrchr(szFileName, _T('\\'));
			if (p2)
				memmove(p2 + 1, p1 + 1, (_tcslen(p1) + 1) * sizeof(TCHAR));
		}
		_tcscpy(szFileName + _tcslen(szFileName) - 4, L"_dbg.txt");
		g_hfDbgText = hf = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		if(hf != INVALID_HANDLE_VALUE) {
			SetFilePointer(hf, 0, NULL, FILE_BEGIN);
#ifdef _UNICODE
			WORD w = 0xfeff;
			DWORD cb;
			WriteFile(hf, &w, sizeof(WORD), &cb, NULL);
#endif
		}
	}
	if(hf != INVALID_HANDLE_VALUE) {
		DWORD cb;
		WriteFile(hf, szBuffer, _tcslen(szBuffer) * sizeof(TCHAR), &cb, NULL);
	}
}
#else	//!_DEBUG
#define TRACE	NOP_FUNCTION
//↓PSDK 2003R2のwinnt.h
//#ifndef NOP_FUNCTION
//#if (_MSC_VER >= 1210)
//#define NOP_FUNCTION __noop
//#else
//#define NOP_FUNCTION (void)0
//#endif
//#endif
#endif	//_DEBUG

//TRACEマクロ
//使用例: TRACE(_T("cx: %d\n"), cx);
#ifdef USE_TRACE
#define TRACE2	_Trace2
#define TRACE2_STR	_Trace2_Str
#define TRACE2_BIN	_Trace2_Bin
#include <stdarg.h>	//va_list
#include <stdio.h>	//_vsnwprintf

static void _Trace2(LPCTSTR pszFormat, ...)
{
	CCriticalSectionLock __lock;
	va_list argptr;
	va_start(argptr, pszFormat);
	//w(v)sprintfは1024文字以上返してこない
	TCHAR szBuffer[1024];
	wvsprintf(szBuffer, pszFormat, argptr);
	OutputDebugString(szBuffer);
}

static void _Trace2_Bin(LPWSTR func, int line, LPVOID lpString, UINT cbString)
{
	const PBYTE srcp = (const PBYTE)lpString;
	WCHAR buf[0x1000];
	LPWSTR p = buf;
	for (UINT i = 0; i < 32 && i < cbString; ++i) {
		wsprintf(p, L"%02x ", srcp[i]);
		p += lstrlen(p);
	}
	*p = 0;
	TRACE(_T("%s %d: %d %08x %s\n"), func, line, cbString, lpString, buf);
}

static void _Trace2_Str(LPWSTR func, int line, LPCWSTR lpString, UINT cbString)
{
	WCHAR buf[0x1000];
	UINT len = Min(cbString, countof(buf) - 1);
	lstrcpyn(buf, lpString, len + 1);
	buf[countof(buf) - 1] = 0;
	TRACE(_T("%s %d: %d %s\n"), func, line, cbString, buf);
	LPWSTR p = buf;
	for (UINT i = 0; i < 32 && i < cbString; ++i) {
		wsprintf(p, L"%04x ", lpString[i]);
		p += lstrlen(p);
	}
	TRACE(_T("%s %d: %d %08x %s\n"), func, line, cbString, lpString, buf);
}

#else	//!USE_TRACE
#define TRACE2	NOP_FUNCTION
#define TRACE2_STR	NOP_FUNCTION
#define TRACE2_BIN	NOP_FUNCTION
#endif	//USE_TRACE

#ifdef _DEBUG

#if 0 /* disable GetClockCount */
FORCEINLINE static __int64 GetClockCount()
{
	LARGE_INTEGER cycles;
	__asm {
		rdtsc
		mov cycles.LowPart,  eax
		mov cycles.HighPart, edx
	}
	return cycles.QuadPart;
}

//使用例
//{
//   CDebugElapsedCounter _cntr("hogehoge");
//     : (適当な処理)
//}
//出力例: "hogehoge: 10000 clocks"
class CDebugElapsedCounter
{
private:
	__int64 m_ilClk;
	LPCSTR m_pszName;
public:
	CDebugElapsedCounter(LPCSTR psz)
		: m_ilClk(GetClockCount())
		, m_pszName(psz)
	{
	}
	~CDebugElapsedCounter()
	{
		TRACE(_T("%hs: %u clocks\n"), m_pszName, (DWORD)(GetClockCount() - m_ilClk));
	}
};
#endif

#else
class CDebugElapsedCounter
{
public:
	CDebugElapsedCounter(LPCSTR psz)
	{
	}
};
#endif
