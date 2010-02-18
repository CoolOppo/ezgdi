HOOK_DEFINE(
   kernel32,
   CreateProcessA, 
   BOOL, 
   (
      LPCSTR lpApp,
      LPSTR lpCmd,
      LPSECURITY_ATTRIBUTES pa,
      LPSECURITY_ATTRIBUTES ta,
      BOOL bInherit,
      DWORD dwFlags,
      LPVOID lpEnv,
      LPCSTR lpDir,
      LPSTARTUPINFOA psi,
      LPPROCESS_INFORMATION ppi
   ),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi)
)

HOOK_DEFINE(
   kernel32,
   CreateProcessW,
   BOOL,
   (
      LPCWSTR lpApp,
      LPWSTR lpCmd,
      LPSECURITY_ATTRIBUTES pa,
      LPSECURITY_ATTRIBUTES ta,
      BOOL bInherit,
      DWORD dwFlags,
      LPVOID lpEnv,
      LPCWSTR lpDir,
      LPSTARTUPINFOW psi,
      LPPROCESS_INFORMATION ppi
   ),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi)
)

HOOK_DEFINE(
   user32,
   DrawStateA,
   BOOL,
   (
      HDC hdc,
      HBRUSH hbr,
      DRAWSTATEPROC lpOutputFunc,
      LPARAM lData,
      WPARAM wData,
      int x,
      int y,
      int cx,
      int cy,
      UINT uFlags
   ),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags)
)

HOOK_DEFINE(
   user32,
   DrawStateW,
   BOOL,
   (
      HDC hdc,
      HBRUSH hbr,
      DRAWSTATEPROC lpOutputFunc,
      LPARAM lData,
      WPARAM wData,
      int x,
      int y,
      int cx,
      int cy,
      UINT uFlags
   ),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags)
)

HOOK_DEFINE(
   gdi32,
   GetTextExtentPointA,
   BOOL,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize)
)

HOOK_DEFINE(
   gdi32,
   GetTextExtentPointW,
   BOOL,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize)
)

HOOK_DEFINE(
   gdi32,
   GetTextExtentPoint32A,
   BOOL,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize)
)

HOOK_DEFINE(
   gdi32,
   GetTextExtentPoint32W,
   BOOL,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize)
)

#if 0

HOOK_DEFINE(
   gdi32,
   CreateFontA,
   HFONT,
   (
      int nHeight,
      int nWidth,
      int nEscapement,
      int nOrientation,
      int fnWeight,
      DWORD fdwItalic,
      DWORD fdwUnderline,
      DWORD fdwStrikeOut,
      DWORD fdwCharSet,
      DWORD fdwOutputPrecision,
      DWORD fdwClipPrecision,
      DWORD fdwQuality,
      DWORD fdwPitchAndFamily,
      LPCSTR  lpszFace
   ),
   (
      nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
      fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
      fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace
   )
)

HOOK_DEFINE(
   gdi32,
   CreateFontW,
   HFONT,
   (
      int nHeight,
      int nWidth,
      int nEscapement,
      int nOrientation,
      int fnWeight,
      DWORD fdwItalic,
      DWORD fdwUnderline,
      DWORD fdwStrikeOut,
      DWORD fdwCharSet,
      DWORD fdwOutputPrecision,
      DWORD fdwClipPrecision,
      DWORD fdwQuality,
      DWORD fdwPitchAndFamily,
      LPCWSTR lpszFace
   ),
   (
      nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
      fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
      fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace
   )
)

HOOK_DEFINE(
   gdi32,
   CreateFontIndirectA,
   HFONT,
   (CONST LOGFONTA *lplf),
   (lplf)
)

HOOK_DEFINE(
   gdi32,
   CreateFontIndirectW,
   HFONT,
   (CONST LOGFONTW *lplf),
   (lplf)
)
#endif

HOOK_DEFINE(
   gdi32,
   CreateFontIndirectExW,
   HFONT,
   (CONST ENUMLOGFONTEXDVW *lpelf),
   (lpelf)
)

HOOK_DEFINE(
   gdi32,
   GetCharWidthW,
   BOOL,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer)
)

HOOK_DEFINE(
   gdi32,
   GetCharWidth32W,
   BOOL,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer)
)

HOOK_DEFINE(
   gdi32,
   TextOutA,
   BOOL,
   (HDC hdc, int nXStart, int nYStart, LPCSTR  lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString)
)

HOOK_DEFINE(
   gdi32,
   TextOutW,
   BOOL,
   (HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString)
)

HOOK_DEFINE(
   gdi32,
   ExtTextOutA,
   BOOL,
   (
      HDC hdc,
      int nXStart,
      int nYStart,
      UINT fuOptions,
      CONST RECT *lprc,
      LPCSTR lpString,
      UINT cbString,
      CONST INT *lpDx
   ),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx)
)

HOOK_DEFINE(
   gdi32,
   ExtTextOutW,
   BOOL,
   (
      HDC hdc,
      int nXStart,
      int nYStart,
      UINT fuOptions,
      CONST RECT *lprc,
      LPCWSTR lpString,
      UINT cbString,
      CONST INT *lpDx
   ),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx)
)

#if 0

HOOK_DEFINE(
   usp10,
   ScriptItemize,
   HRESULT,
   (
      const WCHAR* pwcInChars,
      int cInChars,
      int cMaxItems,
      const SCRIPT_CONTROL* psControl,
      const SCRIPT_STATE* psState,
      SCRIPT_ITEM* pItems,
      int* pcItems
   ),
   (pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems)
)

HOOK_DEFINE(
   usp10,
   ScriptShape,
   HRESULT,
   (
      HDC hdc,
      SCRIPT_CACHE* psc,
      const WCHAR* pwcChars,
      int cChars,
      int cMaxGlyphs,
      SCRIPT_ANALYSIS* psa,
      WORD* pwOutGlyphs,
      WORD* pwLogClust,
      SCRIPT_VISATTR* psva,
      int* pcGlyphs
   ),
   (hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs)
)

HOOK_DEFINE(
   usp10,
   ScriptTextOut,
   HRESULT,
   (
      const HDC hdc,
      SCRIPT_CACHE* psc,
      int x,
      int y,
      UINT fuOptions,
      const RECT* lprc,
      const SCRIPT_ANALYSIS* psa,
      const WCHAR* pwcReserved,
      int iReserved,
      const WORD* pwGlyphs,
      int cGlyphs,
      const int* piAdvance,
      const int* piJustify,
      const GOFFSET* pGoffset
   ),
   (
      hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved,
      pwGlyphs, cGlyphs, piAdvance, piJustify, pGoffset
   )
)

#endif

