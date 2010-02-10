HOOK_DEFINE(
   BOOL, CreateProcessA, 
   (LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi),
   "kernel32.dll"
)

HOOK_DEFINE(
   BOOL, CreateProcessW,
   (LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi),
   (lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi),
   "kernel32.dll"
)

HOOK_DEFINE(
   BOOL, DrawStateA,
   (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags),
   "user32.dll"
)

HOOK_DEFINE(
   BOOL, DrawStateW,
   (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags),
   (hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags),
   "user32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPointA,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPointW,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPoint32A,
   (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetTextExtentPoint32W,
   (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize),
   (hdc, lpString, cbString, lpSize),
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontA,
   (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR  lpszFace),
   (nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace),
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontW,
   (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace),
   (nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace),
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectA,
   (CONST LOGFONTA *lplf),
   (lplf),
   "gdi32.dll"
)

HOOK_DEFINE(
   HFONT, CreateFontIndirectW,
   (CONST LOGFONTW *lplf),
   (lplf),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetCharWidthW,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, GetCharWidth32W,
   (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer),
   (hdc, iFirstChar, iLastChar, lpBuffer),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, TextOutA,
   (HDC hdc, int nXStart, int nYStart, LPCSTR  lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, TextOutW,
   (HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString),
   (hdc, nXStart, nYStart, lpString, cbString),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, ExtTextOutA,
   (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCSTR  lpString, UINT cbString, CONST INT *lpDx),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx),
   "gdi32.dll"
)

HOOK_DEFINE(
   BOOL, ExtTextOutW,
   (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbString, CONST INT *lpDx),
   (hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx),
   "gdi32.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptItemize,
   (const WCHAR* pwcInChars, int cInChars, int cMaxItems, const SCRIPT_CONTROL* psControl, const SCRIPT_STATE* psState, SCRIPT_ITEM* pItems, int* pcItems),
   (pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems),
   "usp10.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptShape,
   (HDC hdc, SCRIPT_CACHE* psc, const WCHAR* pwcChars, int cChars, int cMaxGlyphs, SCRIPT_ANALYSIS* psa, WORD* pwOutGlyphs, WORD* pwLogClust, SCRIPT_VISATTR* psva, int* pcGlyphs),
   (hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs),
   "usp10.dll"
)

HOOK_DEFINE(
   HRESULT, ScriptTextOut,
   (const HDC hdc, SCRIPT_CACHE* psc, int x, int y, UINT fuOptions, const RECT* lprc, const SCRIPT_ANALYSIS* psa, const WCHAR* pwcReserved, int iReserved, const WORD* pwGlyphs, int cGlyphs, const int* piAdvance, const int* piJustify, const GOFFSET* pGoffset),
   (hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs, piAdvance, piJustify, pGoffset),
   "usp10.dll"
)
