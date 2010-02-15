#include "stdafx.h"

#include "override.h"
#include "ft.h"
#include "fteng.h"

#define _GDIPP_EXE
#include "supinfo.h"

#define CCH_MAX_STACK   256

BOOL FreeTypeGetTextExtentPoint(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize, const FREETYPE_PARAMS* params);

HFONT GetCurrentFont(HDC hdc)
{
   HFONT hCurFont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
   if (!hCurFont) {
      // NULLの場合はSystemを設定しておく
      hCurFont = (HFONT)GetStockObject(SYSTEM_FONT);
   }
   return hCurFont;
}

// 有効なDCかどうかをチェックする
BOOL IsValidDC(HDC hdc)
{
   // 優先順位
   // 1. × プリンタ
   // 2. × MM_TEXT以外
   // 3. × Excludeセクション
   // 4. × MaxHeight超え
   // 5. ○ ForceChangeFont
   // 6. × ビットマップフォント

   // 1. プリンタを弾く
   if (GetDeviceCaps(hdc, TECHNOLOGY) != DT_RASDISPLAY) {
      return FALSE;
   }

   // 2. MM_TEXT以外を弾く
   //    (OperaがMM_ANISOTROPICで文字を描画するので廃止)
   //if (GetMapMode(hdc) != MM_TEXT) {
   // return FALSE;
   //}

   // ここでフォントチェックも行う
   // 3. Excludeセクションを弾く
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   WCHAR szFaceName[LF_FACESIZE];
   if (!GetTextFaceW(hdc, LF_FACESIZE, szFaceName) || pSettings->IsFontExcluded(szFaceName)) {
      return FALSE;
   }

   OUTLINETEXTMETRIC otm = { sizeof(OUTLINETEXTMETRIC) };
   TEXTMETRIC& tm = otm.otmTextMetrics;
   const BOOL bOutline = !!GetOutlineTextMetrics(hdc, otm.otmSize, &otm);
   if (!bOutline) {
      if (!GetTextMetrics(hdc, &tm)) {
         return FALSE;
      }
   }

   // 4. MaxHeight超えを弾く
   if (pSettings->MaxHeight() && tm.tmHeight > pSettings->MaxHeight()) {
      return FALSE;
   }

   return TRUE;

   // 5. ForceChangeFontを優先
   if (pSettings->GetForceFontName()) {
      return TRUE;
   }

   // 6. ビットマップフォントを弾く
   return bOutline;
}

BOOL IsProcessExcluded()
{
// if (GetModuleHandle(NULL) == GetModuleHandle(_T("gdi++.exe"))) {
//    return TRUE;
// }

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->IsInclude()) {
      if (pSettings->IsProcessIncluded()) {
         return FALSE;
      }
      return TRUE;
   }

   if (pSettings->IsProcessExcluded()) {
      return TRUE;
   }
   return FALSE;
}

void AddFontToFT(LPCTSTR lpszFace, int weight, bool italic)
{
// CCriticalSectionLock __lock;
   if (lpszFace) {
      CFontFaceNamesEnumerator fn(lpszFace);
      for ( ; !fn.atend(); fn.next()) {
         g_pFTEngine->AddFont(fn, weight, italic);
      }
   }
}

//切り上げ除算
int div_ceil(int a, int b)
{
   if(a % b)
      return (a>0)? (a/b+1): (a/b-1);
   return a / b;
}

template <typename _TCHAR, class _Function>
BOOL _GetTextExtentPoint32AorW(HDC hdc, _TCHAR* lpString, int cbString, LPSIZE lpSize, _Function fn)
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
      || !IsValidDC(hdc) || !lpString || cbString <= 0) {
      return fn(hdc, lpString, cbString, lpSize);
   }

   FREETYPE_PARAMS params(0, hdc);

   if(FreeTypeGetTextExtentPoint(hdc, lpString, cbString, lpSize, &params)) {
      return TRUE;
   }
   else
      return fn(hdc, lpString, cbString, lpSize);
}

//firefox
BOOL WINAPI IMPL_GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize)
{
   return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize, ORIG_GetTextExtentPoint32A);
}

BOOL WINAPI IMPL_GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize)
{
   return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize, ORIG_GetTextExtentPoint32W);
}

BOOL WINAPI IMPL_GetTextExtentPointA(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize)
{
   return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize, ORIG_GetTextExtentPoint32A);
}

BOOL WINAPI IMPL_GetTextExtentPointW(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize)
{
   return _GetTextExtentPoint32AorW(hdc, lpString, cbString, lpSize, ORIG_GetTextExtentPoint32W);
}

BOOL WINAPI IMPL_GetCharWidthW(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
      || !IsValidDC(hdc) || !lpBuffer || !FreeTypeGetCharWidth(hdc, iFirstChar, iLastChar, lpBuffer)) {
      return ORIG_GetCharWidthW(hdc, iFirstChar, iLastChar, lpBuffer);
   }
   return TRUE;
}

BOOL WINAPI IMPL_GetCharWidth32W(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32
      || !IsValidDC(hdc) || !lpBuffer || !FreeTypeGetCharWidth(hdc, iFirstChar, iLastChar, lpBuffer)) {
      return ORIG_GetCharWidth32W(hdc, iFirstChar, iLastChar, lpBuffer);
   }
   return TRUE;
}

BOOL WINAPI IMPL_CreateProcessA(LPCSTR lpApp, LPSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCSTR lpDir, LPSTARTUPINFOA psi, LPPROCESS_INFORMATION ppi)
{
   return ORIG_CreateProcessA(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi);
}

BOOL WINAPI IMPL_CreateProcessW(LPCWSTR lpApp, LPWSTR lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, LPCWSTR lpDir, LPSTARTUPINFOW psi, LPPROCESS_INFORMATION ppi)
{
   return ORIG_CreateProcessW(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi);
}

#if 0

HFONT WINAPI IMPL_CreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCSTR  lpszFace)
{
   LOGFONTA lf = {
      nHeight,
      nWidth,
      nEscapement,
      nOrientation,
      fnWeight,
      !!fdwItalic,
      !!fdwUnderline,
      !!fdwStrikeOut,
      (BYTE)fdwCharSet,
      (BYTE)fdwOutputPrecision,
      (BYTE)fdwClipPrecision,
      (BYTE)fdwQuality,
      (BYTE)fdwPitchAndFamily,
   };
   if (lpszFace)
      strncpy(lf.lfFaceName, lpszFace, LF_FACESIZE - 1);
   return IMPL_CreateFontIndirectA(&lf);
}

#endif

HFONT WINAPI IMPL_CreateFontW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace)
{
   LOGFONTW lf = {
      nHeight,
      nWidth,
      nEscapement,
      nOrientation,
      fnWeight,
      !!fdwItalic,
      !!fdwUnderline,
      !!fdwStrikeOut,
      (BYTE)fdwCharSet,
      (BYTE)fdwOutputPrecision,
      (BYTE)fdwClipPrecision,
      (BYTE)fdwQuality,
      (BYTE)fdwPitchAndFamily,
   };
   if (lpszFace)
      wcsncpy(lf.lfFaceName, lpszFace, LF_FACESIZE - 1);
   return IMPL_CreateFontIndirectW(&lf);
}

HFONT WINAPI IMPL_CreateFontIndirectA(CONST LOGFONTA *lplfA)
{
   if(!lplfA) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->IsFontExcluded(lplfA->lfFaceName)) {
      return ORIG_CreateFontIndirectA(lplfA);
   }

   LOGFONT lf = {
      lplfA->lfHeight,
      lplfA->lfWidth,
      lplfA->lfEscapement,
      lplfA->lfOrientation,
      lplfA->lfWeight,
      lplfA->lfItalic,
      lplfA->lfUnderline,
      lplfA->lfStrikeOut,
      lplfA->lfCharSet,
      lplfA->lfOutPrecision,
      lplfA->lfClipPrecision,
      lplfA->lfQuality,
      lplfA->lfPitchAndFamily,
   };
   MultiByteToWideChar(CP_ACP, 0, lplfA->lfFaceName, LF_FACESIZE, lf.lfFaceName, LF_FACESIZE);

   LOGFONT* lplf = &lf;
   if (pSettings->CopyForceFont(lf, lf)) {
      lplf = &lf;
   }

   HFONT hf = ORIG_CreateFontIndirectW(lplf);
   if(hf && lplf && !pSettings->LoadOnDemand()) {
      AddFontToFT(lplf->lfFaceName, lplf->lfWeight, !!lplf->lfItalic);
   }
   return hf;
}

HFONT WINAPI IMPL_CreateFontIndirectW(CONST LOGFONTW *lplf)
{
   if(!lplf) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->IsFontExcluded(lplf->lfFaceName)) {
      return ORIG_CreateFontIndirectW(lplf);
   }

   LOGFONT lf;
   if (pSettings->CopyForceFont(lf, *lplf)) {
      lplf = &lf;
   }

   HFONT hf = ORIG_CreateFontIndirectW(lplf);
   if(hf && lplf && !pSettings->LoadOnDemand()) {
      AddFontToFT(lplf->lfFaceName, lplf->lfWeight, !!lplf->lfItalic);
   }
   return hf;
}

BOOL WINAPI IMPL_DrawStateA(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags)
{
   int cchW;
   LPWSTR lpStringW;

   if(lData && uFlags & DSS_DISABLED) {
      switch(uFlags & 0x0f) {
      case DST_TEXT:
      case DST_PREFIXTEXT:
         lpStringW = _StrDupAtoW((LPCSTR)lData, wData ? wData : -1, &cchW);
         if(lpStringW) {
            BOOL ret = IMPL_DrawStateW(hdc, hbr, lpOutputFunc, (LPARAM)lpStringW, cchW, x, y, cx, cy, uFlags);
            free(lpStringW);
            return ret;
         }
         break;
      }
   }
   return ORIG_DrawStateA(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags);
}

//灰色描画をDrawTextへ投げる
//DrawTextは内部でExtTextOutしてくれるので問題なし
BOOL WINAPI IMPL_DrawStateW(HDC hdc, HBRUSH hbr, DRAWSTATEPROC lpOutputFunc, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags)
{
   if(lData && uFlags & DSS_DISABLED) {
      switch(uFlags & 0x0f) {
      case DST_TEXT:
      case DST_PREFIXTEXT:
         {
         //wData==0の時に文字数自動計算
         //他のAPIと違って-1の時ではない
         if(wData == 0) {
            wData = wcslen((LPCWSTR)lData);
         }
         RECT rect = { x, y, x + 10000, y + 10000 };
         //どうやら3DHighLightの上に1pxずらして3DShadowを重ねてるらしい
         int oldbm = SetBkMode(hdc, TRANSPARENT);
         COLORREF oldfg = SetTextColor(hdc, GetSysColor(COLOR_3DHIGHLIGHT));
         //DrawStateとDrawTextではprefixのフラグが逆(APIの設計ダメすぎ)
         const UINT uDTFlags = DT_SINGLELINE | DT_NOCLIP | (uFlags & DST_PREFIXTEXT ? 0 : DT_NOPREFIX);

         OffsetRect(&rect, 1, 1);
         DrawTextW(hdc, (LPCWSTR)lData, wData, &rect, uDTFlags);
         SetTextColor(hdc, GetSysColor(COLOR_3DSHADOW));
         OffsetRect(&rect, -1, -1);
         DrawTextW(hdc, (LPCWSTR)lData, wData, &rect, uDTFlags);
         SetTextColor(hdc, oldfg);
         SetBkMode(hdc, oldbm);
         }
         return TRUE;
      }
   }
   return ORIG_DrawStateW(hdc, hbr, lpOutputFunc, lData, wData, x, y, cx, cy, uFlags);
}

BOOL WINAPI IMPL_TextOutA(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cbString)
{
   return IMPL_ExtTextOutA(hdc, nXStart, nYStart, NULL, NULL, lpString, cbString, NULL);
}

BOOL WINAPI IMPL_TextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString)
{
   return IMPL_ExtTextOutW(hdc, nXStart, nYStart, NULL, NULL, lpString, cbString, NULL);
}

void AnsiDxToUnicodeDx(LPCSTR lpStringA, int cbString, const int* lpDxA, int* lpDxW)
{
   LPCSTR lpEndA = lpStringA + cbString;
   while(lpStringA < lpEndA) {
      *lpDxW = *lpDxA++;
      if(IsDBCSLeadByte(*lpStringA)) {
         *lpDxW += *lpDxA++;
         lpStringA++;
      }
      lpDxW++;
      lpStringA++;
   }
}

// ANSI->Unicodeに変換してExtTextOutWに投げるExtTextOutA
BOOL WINAPI IMPL_ExtTextOutA(HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCSTR lpString, UINT cbString, CONST INT *lpDx)
{
   if (!hdc || !lpString || !cbString) {
      return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }

   //However, if the ANSI version of ExtTextOut is called with ETO_GLYPH_INDEX,
   //the function returns TRUE even though the function does nothing.
   //とりあえずオリジナルに飛ばす
   if (fuOptions & ETO_GLYPH_INDEX)
      return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);

   //HDBENCHチート
// if (lpString && cbString == 7 && pSettings->IsHDBench() && (memcmp(lpString, "HDBENCH", 7) == 0 || memcmp(lpString, "       ", 7) == 0))
//    return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);

   LPWSTR lpszUnicode;
   int bufferLength;
   BOOL result;
   WCHAR szStack[CCH_MAX_STACK];
   int dxStack[CCH_MAX_STACK];

   lpszUnicode = _StrDupExAtoW(lpString, cbString, szStack, CCH_MAX_STACK, &bufferLength);
   if(!lpszUnicode) {
      //メモリ不足: 一応オリジナルに投げとく
      return ORIG_ExtTextOutA(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }

   int* lpDxW = NULL;
   result = FALSE;
   if(lpDx && cbString && _getmbcp()) {
      if (cbString < CCH_MAX_STACK) {
         lpDxW = dxStack;
         ZeroMemory(lpDxW, sizeof(int) * cbString);
      } else {
         lpDxW = (int*)calloc(sizeof(int), cbString);
         if (!lpDxW) {
            goto CleanUp;
         }
      }
      AnsiDxToUnicodeDx(lpString, cbString, lpDx, lpDxW);
      lpDx = lpDxW;
   }

   result = IMPL_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, (LPCWSTR)lpszUnicode, bufferLength, lpDx);

CleanUp:
   if (lpszUnicode != szStack)
      free(lpszUnicode);
   if (lpDxW != dxStack)
      free(lpDxW);
   return result;
}

typedef enum {
   ETOE_OK           = 0,
   ETOE_CREATEDC     = 1,
   ETOE_SETFONT      = 2,
   ETOE_CREATEDIB    = 3,
   ETOE_FREETYPE     = 4,
   ETOE_INVALIDARG      = 11,
   ETOE_ROTATION     = 12,
   ETOE_LARGESIZE    = 13,
   ETOE_INVALIDHDC      = 14,
   ETOE_ROTATEFONT      = 15,
   ETOE_NOAREA       = 16,
   ETOE_GETTEXTEXTENT   = 17,
} ExtTextOut_ErrorCode;

//例外モドキ
#define ETO_TRY()    ExtTextOut_ErrorCode error = ETOE_OK; {
#define ETO_THROW(code) error = (code); goto _EXCEPTION_THRU
#define ETO_CATCH()     } _EXCEPTION_THRU:

BOOL FreeTypeGetTextExtentPoint(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize, const FREETYPE_PARAMS* params)
{
   WCHAR szStack[CCH_MAX_STACK];

   int cchStringW;
   LPWSTR lpStringW = _StrDupExAtoW(lpString, cbString, szStack, CCH_MAX_STACK, &cchStringW);
   if(!lpStringW) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   BOOL ret = FreeTypeGetTextExtentPoint(hdc, lpStringW, cchStringW, lpSize, params);
   if (lpStringW != szStack)
      free(lpStringW);
   return ret;
}

class CETOBitmap
{
private:
   CBitmapCache&  m_cache;
   HDC            m_hdc;
   HBITMAP        m_hPrevBmp;
   HBITMAP        m_hBmp;
   BYTE*       m_lpPixels;

public:
   CETOBitmap(CBitmapCache& cache)
      : m_cache(cache)
      , m_hdc(NULL)
      , m_hPrevBmp(NULL)
      , m_hBmp(NULL)
      , m_lpPixels(NULL)
   {
   }
   HDC CreateDC()
   {
      m_hdc = m_cache.CreateDC();
      return m_hdc;
   }
   bool CreateDIBandSelect(int cx, int cy)
   {
      m_hBmp = m_cache.CreateDIB(cx, cy, &m_lpPixels);
      if (!m_hBmp) {
         return false;
      }
      m_hPrevBmp = SelectBitmap(m_hdc, m_hBmp);
      return true;
   }
   void RestoreBitmap()
   {
      if (m_hPrevBmp) {
         SelectBitmap(m_hdc, m_hPrevBmp);
         m_hPrevBmp = NULL;
      }
   }
};

// キレイなExtTextOut（当社比）の実装
BOOL WINAPI IMPL_ExtTextOutW(HDC hdc, int nXStart, int nYStart, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbString, CONST INT *lpDx)
{
   if (!hdc || !lpString || !cbString) {
      return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }

   CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
   if(!pTLInfo) {
      return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }

   // サロゲートや合成を含む場合はオリジナルの関数にぶん投げる
   // Uniscribe経由でETO_GLYPH_INDEXフラグ付きで戻ってくる
   // ビットマップフォントやフォールバック時などETO_GLYPH_INDEXが付かないこともあるので再入をチェック
#if USE_DETOURS
   if (!(fuOptions & ETO_GLYPH_INDEX) && !pTLInfo->InUniscribe()
       && ScriptIsComplex(lpString, cbString, SIC_COMPLEX) == S_OK) {
      return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }
#endif

   if (pTLInfo->InExtTextOut()) {
      return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
   }

   CBitmapCache& cache  = pTLInfo->BitmapCache();
   CETOBitmap bmp(cache);

   HDC      hCanvasDC      = NULL;
   HFONT hPrevFont      = NULL;

ETO_TRY();
   //再帰防止フラグ
   pTLInfo->InExtTextOut(true);

   POINT curPos = { nXStart, nYStart };
   POINT destPos;
   SIZE  drawSize;

   HFONT hCurFont    = NULL;
   BOOL  bShadow        = FALSE;

   UINT  align;
   SIZE  textSize;
   SIZE  realSize = { 0 };
   TEXTMETRIC tm = { 0 };
   LOGFONT  lf = { 0 };

   if (!IsValidDC(hdc)) {
      ETO_THROW(ETOE_INVALIDHDC);   // hdc is invalid
   }

   hCanvasDC = bmp.CreateDC();
   if(!hCanvasDC) {
      ETO_THROW(ETOE_CREATEDC);
   }

   // get cursor / xform
   align = GetTextAlign(hdc);
   if(align & TA_UPDATECP) {
      GetCurrentPositionEx(hdc, &curPos);
   }

   //copy font
   //フォント周りはコストが大きく、何度もGetCurrentObjectやGetTextMetricsするのは
   //効率が悪いので1度だけ情報を取得して使い回す
   hCurFont = GetCurrentFont(hdc);
   if (!hCurFont) {
      ETO_THROW(ETOE_SETFONT);
   }
   hPrevFont = SelectFont(hCanvasDC, hCurFont);
   GetTextMetrics(hdc, &tm);
   if (!GetObject(hCurFont, sizeof(LOGFONT), &lf)) {
      ETO_THROW(ETOE_SETFONT);
   }
   if (lf.lfHeight == 0) {
      // lfHeight が 0 だと FreeType で文字が出ない
      lf.lfHeight = tm.tmHeight;
   }

   // FreeTypeを使うかどうかの判定
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   pSettings->CopyForceFont(lf, lf);
   if(pSettings->LoadOnDemand()) {
      //要求時ロード
      AddFontToFT(lf.lfFaceName, lf.lfWeight, !!lf.lfItalic);
   }
   //if(!g_pFTEngine->FontExists(lf.lfFaceName, lf.lfWeight, !!lf.lfItalic)) {
   // ETO_THROW(ETOE_FREETYPE);
   //}

   //背景は予め描画するので透明でよい
   FREETYPE_PARAMS params(fuOptions & ~ETO_OPAQUE, hdc, &lf);

   BITMAP bm;
   HBITMAP hbmpSrc = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
   if(hbmpSrc && GetObject(hbmpSrc, sizeof(BITMAP), &bm) && bm.bmBitsPixel == 1) {
      //文字が潰れるのでモノクロビットマップにはAAを適用させない
      params.ftOptions |= FTO_MONO;
   }

   //GetTextExtentPoint系はSetTextCharacterExtraを必要とするらしい
   SetTextCharacterExtra(hCanvasDC, params.charExtra);
   //text size
   //if(lpDx && !(cbString == 1 && lpDx[0] == 1000)) { // メモ帳＠WinXP対策
   if(lpDx) {
      textSize.cx = 0;
      textSize.cy = tm.tmHeight;
      UINT i;
      if(fuOptions & ETO_PDY) {
         for(i=0; i < (cbString - 1) * 2; textSize.cx += lpDx[i],i+=2);
         realSize.cx = textSize.cx;
      } else {
         //for(UINT i=0; i < cbString; i++) {
         for(i=0; i < cbString - 1; i++) {
            realSize.cx += lpDx[i] + params.charExtra;
         }
         //textSize.cx = realSize.cx + lpDx[cbString - 1] + params.charExtra;
      }
      SIZE sz = { 0 };
      if(!FreeTypeGetTextExtentPoint(hCanvasDC, lpString + cbString - 1, 1, &sz, &params)) {
         ETO_THROW(ETOE_GETTEXTEXTENT);
      }
      realSize.cx += Max(sz.cx, (LONG)lpDx[i]) + params.charExtra;
      textSize.cx = realSize.cx;
   } else {
      if(!FreeTypeGetTextExtentPoint(hCanvasDC, lpString, (int)cbString, &textSize, &params)) {
         ETO_THROW(ETOE_GETTEXTEXTENT);
      }
      realSize.cx = textSize.cx;
   }
   realSize.cy = tm.tmHeight;

   //******************
   //この辺はまだFreeType用の処理が出来ていない。
   //FT_Outline_Get_CBoxでも使えばいいのかなぁ？

   // イタリック時のスラント幅を足す(構造体の名前がABC・・・
   // 参考: http://bugzilla.mozilla.gr.jp/show_bug.cgi?id=3253
   if (tm.tmItalic) {
      ABC abcWidth={0, 0, 0};
      GetCharABCWidthsW(hdc, lpString[cbString-1], lpString[cbString-1], &abcWidth);
      textSize.cx += tm.tmOverhang - abcWidth.abcC;
   }
   //******************

   // 縦書き時の処理
   // まだ実装してないので、暫定的に使えるようにエラーを投げる
   // if ((lf.lfEscapement % 90) != 0)
   if (lf.lfEscapement != 0) {
      ETO_THROW(ETOE_ROTATEFONT);// rotated font
   }
   //trace(L"OrigTextSize=%d %d\n", textSize.cx, textSize.cy);
   //trace(L"OrigCursor=%d %d\n", curPos.x, curPos.y);

   //rectangle
   {
      RECT rc = { 0 };
      const UINT horiz = align & (TA_LEFT|TA_RIGHT|TA_CENTER);
      const UINT vert  = align & (TA_BASELINE|TA_TOP|TA_BOTTOM);

      switch (horiz) {
      case TA_CENTER:
         rc.left  = curPos.x - div_ceil(textSize.cx, 2);
         rc.right = curPos.x + div_ceil(textSize.cx, 2);
         //no move
         break;
      case TA_RIGHT:
         rc.left  = curPos.x - textSize.cx;
         rc.right = curPos.x;
         curPos.x -= realSize.cx;//move pos
         break;
      default:
         rc.left  = curPos.x;
         rc.right = curPos.x + textSize.cx;
         curPos.x += realSize.cx;//move pos
         break;
      }

      switch (vert) {
      case TA_BASELINE:
         rc.top = curPos.y - tm.tmAscent;
         rc.bottom = curPos.y + tm.tmDescent;
         //trace(L"ascent=%d descent=%d\n", metric.tmAscent, metric.tmDescent);
         break;
      case TA_BOTTOM:
         rc.top = curPos.y - textSize.cy;
         rc.bottom = curPos.y;
         break;
      default:
         rc.top = curPos.y;
         rc.bottom = curPos.y + textSize.cy;
         break;
      }

      destPos.x = rc.left;
      destPos.y = rc.top;
      drawSize.cx = textSize.cx;
      drawSize.cy = rc.bottom - rc.top;
   }

   //trace(L"MovedCursor=%d %d\n", curPos.x, curPos.y);
   //trace(L"TargetRect=%d %d %d %d\n", rc.left, rc.top, rc.right, rc.bottom);
   //trace(L"DestPos=%dx%d Size=%dx%d\n", destPos.x, destPos.y, destSize.cx, destSize.cy);
   //trace(L"CanvasPos=%dx%d Size=%dx%d\n", canvasPos.x, canvasPos.y, canvasSize.cx, canvasSize.cy);

   if(drawSize.cx < 1 || drawSize.cy < 1) {
      ETO_THROW(ETOE_NOAREA); //throw no area
   }

   // アバウトな斜体などの文字欠け対応
   drawSize.cx += tm.tmMaxCharWidth;

   //bitmap
   if(!params.IsMono() && pSettings->EnableShadow()) {
      bShadow = true;
   }
   if (!bmp.CreateDIBandSelect(drawSize.cx, drawSize.cy)) {
      ETO_THROW(ETOE_CREATEDIB);
   }

   {
      const BOOL fillrect = (lprc && (fuOptions & ETO_OPAQUE));

      //clear bitmap
      if(fillrect || GetBkMode(hdc) == OPAQUE) {
         const COLORREF  bgcolor = GetBkColor(hdc); //両方とも同じ背景色に
         //DIB直接操作
         SetBkMode(hCanvasDC, OPAQUE);
         SetBkColor(hCanvasDC, bgcolor);

         //RECT rc = { 0, 0, textSize.cx, textSize.cy };
         RECT rc = { 0, 0, drawSize.cx, drawSize.cy };
         cache.FillSolidRect(bgcolor, &rc);

         if(fillrect) {
            ORIG_ExtTextOutW(hdc, 0, 0, ETO_OPAQUE, lprc, NULL, 0, NULL);
         }
      }
      else {
         //if (realSize.cx < textSize.cx) {
         // textSize.cx = realSize.cx;
         // drawSize.cx = realSize.cx;
         //}
         BitBlt(hCanvasDC, 0, 0, drawSize.cx, drawSize.cy, hdc, destPos.x, destPos.y, SRCCOPY);
      }
   }

   //setup
   SetTextAlign(hCanvasDC, TA_LEFT | TA_TOP);
   //debug
   //Dbg_TraceExtTextOutW(nXStart, nYStart, fuOptions, lpString, cbString, lpDx);

   //textout
   SetTextColor(hCanvasDC, GetTextColor(hdc));
   SetBkMode(hCanvasDC, TRANSPARENT);
   int width;
   if (bShadow) {
      //影
      const int* shadow = pSettings->GetShadowParams();
      int xs = shadow[0], ys = shadow[1];
      params.alpha = shadow[2];
      FreeTypeTextOut(hCanvasDC, cache, xs, ys, lpString, cbString, lpDx, &params, width);
      params.alpha = 0;
   }
   if (!FreeTypeTextOut(hCanvasDC, cache, 0, 0, lpString, cbString, lpDx, &params, width)) {
      ETO_THROW(ETOE_FREETYPE);
   }
   drawSize.cx = width;

   //blt + clipping
   if(lprc && (fuOptions & ETO_CLIPPED)) {
      const RECT rcBlt = { destPos.x, destPos.y, destPos.x + drawSize.cx, destPos.y + drawSize.cy };
      RECT rcClip = { 0 };
      IntersectRect(&rcClip, &rcBlt, lprc);
      BitBlt(hdc, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top,
            hCanvasDC, rcClip.left - rcBlt.left, rcClip.top - rcBlt.top, SRCCOPY);
   } else {
      BitBlt(hdc, destPos.x, destPos.y, drawSize.cx, drawSize.cy, hCanvasDC, 0, 0, SRCCOPY);
   }

   if(align & TA_UPDATECP) {
      MoveToEx(hdc, curPos.x, curPos.y, NULL);
   }

ETO_CATCH();
   pTLInfo->InExtTextOut(false);
   bmp.RestoreBitmap();
   if(hPrevFont) {
      SelectFont(hCanvasDC, hPrevFont);
   }
   if(error == ETOE_OK) {
      return TRUE;
   }
   return ORIG_ExtTextOutW(hdc, nXStart, nYStart, fuOptions, lprc, lpString, cbString, lpDx);
}

HRESULT WINAPI IMPL_ScriptItemize(
  const WCHAR* pwcInChars, 
  int cInChars, 
  int cMaxItems, 
  const SCRIPT_CONTROL* psControl, 
  const SCRIPT_STATE* psState, 
  SCRIPT_ITEM* pItems, 
  int* pcItems 
) {
   HRESULT hr = ORIG_ScriptItemize(pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, pcItems);
   if (FAILED(hr))
      return hr;
   //異体字セレクタで始まる実行単位(ラン)を探す
   //最終実行単位の長さを示すための項目は*pcItemsの値に含まれないことに注意
   for (int i = 1; i < *pcItems; ++i) {
      //条件を満たさない実行単位はスキップ
      const int iCharPos = pItems[i].iCharPos;
      if (pItems[i + 1].iCharPos - iCharPos < 2)
         continue; //異体字セレクタの表現には少なくともUnicodeコードポイント2つが必要
      if (pwcInChars[iCharPos] != 0xDB40)
         continue; //上位サロゲートの値が範囲外
      if (pwcInChars[iCharPos + 1] - 0xDD00 >= 0xF0)
         continue; //下位サロゲートの値が範囲外
      //異体字セレクタを1つ前の実行単位に移動する
      pItems[i].iCharPos += 2;
      //長さが0になった場合は実行単位そのものを削除する
      if (pItems[i + 1].iCharPos <= pItems[i].iCharPos) {
         memmove(&pItems[i], &pItems[i + 1], (*pcItems - i) * sizeof SCRIPT_ITEM);
         --*pcItems;
         --i; //削除した分のつじつま合わせ
      }
   }
   return hr;
}

HRESULT WINAPI IMPL_ScriptShape(
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
) {
   //実行単位の末尾が異体字シーケンスでなければ何もしない
   WORD vsindex;
   if (cChars < 3 || cMaxGlyphs < 1 || pwcChars[cChars - 2] != 0xDB40 || (vsindex = pwcChars[cChars - 1] - 0xDD00) >= 0xF0)
      return ORIG_ScriptShape(hdc, psc, pwcChars, cChars, cMaxGlyphs, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);

   if (!hdc)
      return E_PENDING; //判定にはHDCが必要

   //異体字セレクタを削ってシェーピングエンジンに渡す
   HRESULT hr = ORIG_ScriptShape(hdc, psc, pwcChars, cChars - 2, cMaxGlyphs - 1, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);
   if (FAILED(hr) || psa->fNoGlyphIndex)
      return hr;

   //最終グリフを置き換える
   WORD high;
   WORD low = pwcChars[cChars - 3] - 0xDC00;
   int baseChar;
   if (cChars >= 4 && (high = pwcChars[cChars - 4] - 0xD800) < 0x400 && low < 0x400)
      baseChar = ((high << 10) | low) + 0x10000;
   else
      baseChar = pwcChars[cChars - 3];
   if (*pcGlyphs > 0) {
      FreeTypeSubstGlyph(hdc, vsindex, baseChar, cChars, psa, pwOutGlyphs, pwLogClust, psva, pcGlyphs);
   }
   return hr;
}

HRESULT WINAPI IMPL_ScriptTextOut(
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
) {
   CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
   if (pTLInfo) {
      pTLInfo->InUniscribe(true);
      pTLInfo->InExtTextOut(false);
    }
   HRESULT hr = ORIG_ScriptTextOut(hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved,
      pwGlyphs, cGlyphs, piAdvance, piJustify, pGoffset);

   if (pTLInfo)
      pTLInfo->InUniscribe(false);

   return hr;
}

