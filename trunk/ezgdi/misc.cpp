#include "stdafx.h"

CRITICAL_SECTION CCriticalSectionLock::m_cs;

#ifdef _UNICODE
#undef CharPrev
#define CharPrev(s, c)  ((c) - 1)
#endif

LPWSTR _StrDupExAtoW(LPCSTR pszMB, int cchMB /*= -1*/, LPWSTR pszStack /*= NULL*/, int cchStack /*= 0*/, int* pcchWC /*= NULL*/)
{
   int _cchWC;
   if (!pcchWC) {
      pcchWC = &_cchWC;
   }
   *pcchWC = 0;

   if (!pszMB) {
      return NULL;
   }
   if (cchMB == -1) {
      cchMB = strlen(pszMB);
   }
   const int cchWC = MultiByteToWideChar(CP_ACP, 0, pszMB, cchMB, NULL, 0);
   if(cchWC < 0) {
      return NULL;
   }

   LPWSTR pszWC;
   if(cchWC < cchStack && pszStack) {
      pszWC = pszStack;
      ZeroMemory(pszWC, sizeof(WCHAR) * (cchWC + 1));
   } else {
      pszWC = (LPWSTR)calloc(sizeof(WCHAR), cchWC + 1);
      if (!pszWC) {
         return NULL;
      }
   }
   MultiByteToWideChar(CP_ACP, 0, pszMB, cchMB, pszWC, cchWC);
   pszWC[cchWC] = L'\0';
   *pcchWC = cchWC;
   return pszWC;
}
