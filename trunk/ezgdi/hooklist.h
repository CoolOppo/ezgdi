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
