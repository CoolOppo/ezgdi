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
   bool        m_bPadding[3];

public:
   CThreadLocalInfo()
      : m_bInExtTextOut(false)
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
};

extern CTlsData<CThreadLocalInfo>   g_TLInfo;

BOOL IsProcessExcluded();
HFONT GetCurrentFont(HDC hdc);
void AddFontToFT(HFONT hf, LPCTSTR lpszFace, int weight, bool italic);
void AddFontToFT(LPCTSTR lpszFace, int weight, bool italic);