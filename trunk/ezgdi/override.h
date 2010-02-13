#pragma once

#include "settings.h"

#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   extern rettype (WINAPI * ORIG_##name) argtype; \
   extern rettype WINAPI IMPL_##name argtype;
#include "hooklist.h"
#undef HOOK_DEFINE

class CThreadLocalInfo
{
private:
   CBitmapCache   m_bmpCache;
   bool        m_bInExtTextOut;
   bool        m_bInUniscribe;
   bool        m_bPadding[2];

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

extern CTlsData<CThreadLocalInfo>   g_TLInfo;

BOOL IsProcessExcluded();
FORCEINLINE BOOL IsOSXPorLater()
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   return pSettings->IsWinXPorLater();
}

HFONT GetCurrentFont(HDC hdc);
void AddFontToFT(HFONT hf, LPCTSTR lpszFace, int weight, bool italic);
void AddFontToFT(LPCTSTR lpszFace, int weight, bool italic);
