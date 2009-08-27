
#pragma once

#include "common.h"
#include "tlsdata.h"

#include "cache.h"
#include "settings.h"

class CThreadLocalInfo
{
private:
	CBitmapCache	m_bmpCache;
	bool			m_bInExtTextOut;
	bool			m_bInUniscribe;
	bool			m_bPadding[2];

public:
	CThreadLocalInfo()
		: m_bInExtTextOut(false), m_bInUniscribe(false)
	{
	}

	CBitmapCache& BitmapCache()
	{
		return m_bmpCache;
	}

	void InExtTextOut(bool b)
	{
		m_bInExtTextOut = b;
	}
	bool InExtTextOut() const
	{
		return m_bInExtTextOut;
	}

	void InUniscribe(bool b)
	{
		m_bInUniscribe = b;
	}
	bool InUniscribe() const
	{
		return m_bInUniscribe;
	}
};

extern CTlsData<CThreadLocalInfo>	g_TLInfo;

BOOL IsProcessExcluded();
FORCEINLINE BOOL IsOSXPorLater()
{
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	return pSettings->IsWinXPorLater();
}

#ifdef USE_DETOURS

#define HOOK_DEFINE(rettype, name, argtype) \
	extern rettype (WINAPI * ORIG_##name) argtype; \
	extern rettype WINAPI IMPL_##name argtype;
#include "hooklist.h"
#undef HOOK_DEFINE

#else

#define HOOK_DEFINE(rettype, name, argtype) \
	extern rettype WINAPI IMPL_##name argtype;
#include "hooklist.h"
#undef HOOK_DEFINE
#define ORIG_GetTextExtentPoint32A GetTextExtentPoint32A
#define ORIG_GetTextExtentPoint32W GetTextExtentPoint32W
#define ORIG_CreateProcessA CreateProcessA
#define ORIG_CreateProcessW CreateProcessW
#define ORIG_GetCharWidth32W GetCharWidth32W
#define ORIG_GetCharWidthW GetCharWidthW
#define ORIG_CreateFontIndirectA CreateFontIndirectA
#define ORIG_CreateFontIndirectW CreateFontIndirectW
#define ORIG_DrawStateA DrawStateA
#define ORIG_DrawStateW DrawStateW
#define ORIG_ExtTextOutA ExtTextOutA
#define ORIG_ExtTextOutW ExtTextOutW
#define ORIG_ScriptItemize ScriptItemize
#define ORIG_ScriptShape ScriptShape
#define ORIG_ScriptTextOut ScriptTextOut

#endif

HFONT GetCurrentFont(HDC hdc);
void AddFontToFT(HFONT hf, LPCTSTR lpszFace, int weight, bool italic);
void AddFontToFT(LPCTSTR lpszFace, int weight, bool italic);

//EOF
