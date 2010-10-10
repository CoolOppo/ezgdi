HOOK_DEFINE(
   BOOL, CreateProcessA, 
   (LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi),
   "lpApp(%s), lpCmd(%s), pa(%p), ta(%p), bInherit(%d), dwFlags(%ld), lpEnv(%p), lpDir(%s), psi(%p), ppi(%p)",
   "kernel32.dll"
)

HOOK_DEFINE(
   BOOL, CreateProcessW,
   (LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi),
   "lpApp(%ls), lpCmd(%ls), pa(%p), ta(%p), bInherit(%d), dwFlags(%ld), lpEnv(%p), lpDir(%ls), psi(%p), ppi(%p)",
   "kernel32.dll"
)

HOOK_DEFINE(
   BOOL, DrawStateA,
   (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags),
   "hdc(%p), hbr(%p), lpOutputFunc(%p), lData(%ld), wData(%ld), x(%d), y(%d), cx(%d), cy(%d), uFlags(%x)",
   "user32.dll"
)

HOOK_DEFINE(
   BOOL, DrawStateW,
   (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags),
   "hdc(%p), hbr(%p), lpOutputFunc(%p), lData(%ld), wData(%ld), x(%d), y(%d), cx(%d), cy(%d), uFlags(%x)",
   "user32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPointA,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "hdc(%p), lpString(%s), cbString(%d), lpSize(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPointW,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "hdc(%p), lpString(%ls), cbString(%d), lpSize(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPoint32A,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "hdc(%p), lpString(%s), cbString(%d), lpSize(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPoint32W,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "hdc(%p), lpString(%ls), cbString(%d), lpSize(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontA,
   (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR  lpszFace),
   (nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace),
   "nHeight(%d), nWidth(%d), nEscapement(%d), nOrientation(%d), fnWeight(%d), fdwItalic(%ld), fdwUnderline(%ld), fdwStrikeOut(%ld), fdwCharSet(%ld), fdwOutputPrecision(%ld), fdwClipPrecision(%ld), fdwQuality(%ld), fdwPitchAndFamily(%ld), lpszFace(%s)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontW,
   (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace),
   (nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace),
   "nHeight(%d), nWidth(%d), nEscapement(%d), nOrientation(%d), fnWeight(%d), fdwItalic(%ld), fdwUnderline(%ld), fdwStrikeOut(%ld), fdwCharSet(%ld), fdwOutputPrecision(%ld), fdwClipPrecision(%ld), fdwQuality(%ld), fdwPitchAndFamily(%ld), lpszFace(%ls)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectA,
   (CONST LOGFONTA *lplf),
   (lplf),
   "lplf(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectW,
   (CONST LOGFONTW *lplf),
   (lplf),
   "lplf(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectExA,
   (CONST ENUMLOGFONTEXDVA *lpelf),
   (lpelf),
   "lpelf(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectExW,
   (CONST ENUMLOGFONTEXDVW *lpelf),
   (lpelf),
   "lpelf(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetCharWidthW,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer),
   "hdc(%p), iFirstChar(%u), iLastChar(%u), lpBuffer(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetCharWidth32W,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer),
   "hdc(%p), iFirstChar(%u), iLastChar(%u), lpBuffer(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, TextOutA,
   (HDC hdc, int nXStart, int nYStart, LPCSTR  lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString),
   "hdc(%p), nXStart(%d), nYStart(%d), lpString(%p), cbString(%d)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, TextOutW,
   (HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString),
   "hdc(%p), nXStart(%d), nYStart(%d), lpString(%p), cbString(%d)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, ExtTextOutA,
   (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCSTR  lpString, UINT cbString, CONST INT *lpDx),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx),
   "hdc(%p), nXStart(%d), nYStart(%d), fuOptions(%u), lprc(%p), lpString(%p), cbString(%u), lpDx(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, ExtTextOutW,
   (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbString, CONST INT *lpDx),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx),
   "hdc(%p), nXStart(%d), nYStart(%d), fuOptions(%u), lprc(%p), lpString(%p), cbString(%u), lpDx(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptItemize,
   (const WCHAR* pwcInChars, int cInChars, int cMaxItems, const SCRIPT_CONTROL* psControl, const SCRIPT_STATE* psState, SCRIPT_ITEM* pItems, int* pcItems),
   (pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems),
   "pwcInChars(%pLS), cInChars(%d), cMaxItems(%d), psControl(%p), psState(%p), pItems(%p), pcItems(%p)",
   "usp10.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptShape,
   (HDC hdc, SCRIPT_CACHE* psc, const WCHAR* pwcChars, int cChars, int cMaxGlyphs, SCRIPT_ANALYSIS* psa, WORD* pwOutGlyphs, WORD* pwLogClust, SCRIPT_VISATTR* psva, int* pcGlyphs),
   (hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs),
   "hdc(%p), psc(%p), pwcChars(%pLS), cChars(%d), cMaxGlyphs(%d), psa(%p), pwOutGlyphs(%p), pwLogClust(%p), psva(%p), pcGlyphs(%p)",
   "usp10.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptTextOut,
   (const HDC hdc, SCRIPT_CACHE* psc, int x, int y, UINT fuOptions, const RECT* lprc, const SCRIPT_ANALYSIS* psa, const WCHAR* pwcReserved, int iReserved, const WORD* pwGlyphs, int cGlyphs, const int* piAdvance, const int* piJustify, const GOFFSET* pGoffset),
   (hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs, piAdvance, piJustify, pGoffset),
   "hdc(%p), psc(%p), x(%d), y(%d), fuOptions(%u), lprc(%p), psa(%p), pwcReserved(%pLS), iReserved(%p), pwGlyphs(%p), cGlyphs(%d), piAdvance(%p), piJustify(%p), pGoffset(%p)",
   "usp10.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentExPointW,
   (HDC hdc, LPCTSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize),
   (hdc, lpszStr, cchString, nMaxExtent, lpnFit, alpDx, lpSize),
   "hdc(%p), lpszStr(%p), cchString(%d), nMaxExtent(%d), lpnFit(%p), alpDx(%p), lpSize(%p)",
   "gdi32.dll"
)

HOOK_DEFINE(
   DWORD, GetCharacterPlacementW,
   (HDC hdc, LPCTSTR lpString, int nCount, int nMaxExtent, LPGCP_RESULTS lpResults, DWORD dwFlags),
   (hdc, lpString, nCount, nMaxExtent, lpResults, dwFlags),
   "hdc(%p), lpString(%p), nCount(%d), nMaxExtent(%d), lpResults(%p), dwFlags(%x)",
   "gdi32.dll"
)
