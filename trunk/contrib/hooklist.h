// ここにフックするAPIを列挙していく。
// ORIG_foo を使いつつ、IMPL_foo を実装すること。

/*
HOOK_DEFINE(DWORD, GetTabbedTextExtentA, (HDC hdc, LPCSTR lpString, int nCount, int nTabPositions, CONST LPINT lpnTabStopPositions))
HOOK_DEFINE(DWORD, GetTabbedTextExtentW, (HDC hdc, LPCWSTR lpString, int nCount, int nTabPositions, CONST LPINT lpnTabStopPositions))

HOOK_DEFINE(BOOL, GetTextExtentExPointA, (HDC hdc, LPCSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize))
HOOK_DEFINE(BOOL, GetTextExtentExPointW, (HDC hdc, LPCWSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize))
HOOK_DEFINE(BOOL, GetTextExtentExPointI, (HDC hdc, LPWORD pgiIn, int cgi, int nMaxExtent, LPINT lpnFit, LPINT lpDx, LPSIZE lpSize))

HOOK_DEFINE(BOOL, GetTextExtentPointI, (HDC hdc, LPWORD pgiIn, int cgi, LPSIZE lpSize))
*/

HOOK_DEFINE(BOOL, CreateProcessA, (LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi))
HOOK_DEFINE(BOOL, CreateProcessW, (LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi))

HOOK_DEFINE(BOOL, DrawStateA, (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags))
HOOK_DEFINE(BOOL, DrawStateW, (HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags))

HOOK_DEFINE(BOOL, GetTextExtentPointA, (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize))
HOOK_DEFINE(BOOL, GetTextExtentPointW, (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize))
HOOK_DEFINE(BOOL, GetTextExtentPoint32A, (HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize))
HOOK_DEFINE(BOOL, GetTextExtentPoint32W, (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize))

// DrawTextA,W
// DrawTextExA,W
// TabbedTextOutA,W
// TextOutA,W
// ExtTextOutA
// は内部で ExtTextOutWを呼んでるから ExtTextOutW だけ実装すればOK。←XPの場合
// Windows 2000 でも動くようにその他のAPIもフックを掛けておく。

HOOK_DEFINE(HFONT, CreateFontA, (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR  lpszFace))
HOOK_DEFINE(HFONT, CreateFontW, (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace))
HOOK_DEFINE(HFONT, CreateFontIndirectA, (CONST LOGFONTA *lplf))
HOOK_DEFINE(HFONT, CreateFontIndirectW, (CONST LOGFONTW *lplf))

HOOK_DEFINE(BOOL, GetCharWidthW, (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer))
HOOK_DEFINE(BOOL, GetCharWidth32W, (HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer))

HOOK_DEFINE(BOOL, TextOutA, (HDC hdc, int nXStart, int nYStart, LPCSTR  lpString, int cbString))
HOOK_DEFINE(BOOL, TextOutW, (HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString))
HOOK_DEFINE(BOOL, ExtTextOutA, (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCSTR  lpString, UINT cbString, CONST INT *lpDx))
HOOK_DEFINE(BOOL, ExtTextOutW, (HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbString, CONST INT *lpDx))

HOOK_DEFINE(HRESULT, ScriptItemize, (\
  const WCHAR* pwcInChars, \
  int cInChars, \
  int cMaxItems, \
  const SCRIPT_CONTROL* psControl, \
  const SCRIPT_STATE* psState, \
  SCRIPT_ITEM* pItems, \
  int* pcItems \
))

HOOK_DEFINE(HRESULT, ScriptShape, (\
  HDC hdc, \
  SCRIPT_CACHE* psc, \
  const WCHAR* pwcChars, \
  int cChars, \
  int cMaxGlyphs, \
  SCRIPT_ANALYSIS* psa, \
  WORD* pwOutGlyphs, \
  WORD* pwLogClust, \
  SCRIPT_VISATTR* psva, \
  int* pcGlyphs \
))

HOOK_DEFINE(HRESULT, ScriptTextOut, (\
  const HDC hdc, \
  SCRIPT_CACHE* psc, \
  int x, \
  int y, \
  UINT fuOptions, \
  const RECT* lprc, \
  const SCRIPT_ANALYSIS* psa, \
  const WCHAR* pwcReserved, \
  int iReserved, \
  const WORD* pwGlyphs, \
  int cGlyphs, \
  const int* piAdvance, \
  const int* piJustify, \
  const GOFFSET* pGoffset \
))
//EOF
