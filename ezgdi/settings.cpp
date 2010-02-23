#include "stdafx.h"

#include "settings.h"
#include "strtoken.h"
#include <math.h> //pow
#define _GDIPP_DLL
#include "supinfo.h"
#include "fteng.h"


CGdippSettings* CGdippSettings::s_pInstance;


static const TCHAR c_szGeneral[]  = _T("General");
static const TCHAR c_szFreeType[] = _T("FreeType");
#define HINTING_MIN        0
#define HINTING_MAX        2
#define AAMODE_MIN         -1
#define AAMODE_MAX         5
#define GAMMAVALUE_MIN     0.0625f
#define GAMMAVALUE_MAX     10.0f
#define CONTRAST_MIN    0.0625f
#define CONTRAST_MAX    10.0f
#define RENDERWEIGHT_MIN   0.0625f
#define RENDERWEIGHT_MAX   10.0f
#define NWEIGHT_MIN        -64
#define NWEIGHT_MAX        +64
#define BWEIGHT_MIN        -32
#define BWEIGHT_MAX        +32
#define SLANT_MIN       -32
#define SLANT_MAX       +32


CGdippSettings* CGdippSettings::CreateInstance()
{
   CCriticalSectionLock __lock;

   CGdippSettings* pSettings = new CGdippSettings;
   CGdippSettings* pOldSettings = reinterpret_cast<CGdippSettings*>(InterlockedExchangePointer(reinterpret_cast<void**>(&s_pInstance), pSettings));
   _ASSERTE(pOldSettings == NULL);
   return pSettings;
}

void CGdippSettings::DestroyInstance()
{
   CCriticalSectionLock __lock;

   CGdippSettings* pSettings = reinterpret_cast<CGdippSettings*>(InterlockedExchangePointer(reinterpret_cast<void**>(&s_pInstance), NULL));
   if (pSettings) {
      delete pSettings;
   }
}

const CGdippSettings* CGdippSettings::GetInstance()
{
   CCriticalSectionLock __lock;
   CGdippSettings* pSettings = s_pInstance;
   _ASSERTE(pSettings != NULL);

   if (!pSettings->m_bDelayedInit) {
      pSettings->DelayedInit();
   }
   return pSettings;
}

const CGdippSettings* CGdippSettings::GetInstanceNoInit()
{
   CCriticalSectionLock __lock;
   CGdippSettings* pSettings = s_pInstance;
   _ASSERTE(pSettings != NULL);
   return pSettings;
}

void CGdippSettings::DelayedInit()
{
   if (!g_pFTEngine) {
      return;
   }

   //ForceChangeFont
   if (m_szForceChangeFont[0]) {
      HDC hdc = GetDC(NULL);
      EnumFontFamilies(hdc, m_szForceChangeFont, EnumFontFamProc, reinterpret_cast<LPARAM>(this));
      ReleaseDC(NULL, hdc);
   }

   //FontLink
   if (FontLink()) {
      m_fontlinkinfo.init();
   }

   //FontSubstitutes
   CFontSubstitutesIniArray arrFontSubstitutes;
   AddListFromSection(_T("FontSubstitutes"), m_szFileName, arrFontSubstitutes);
   m_FontSubstitutesInfo.init(m_nFontSubstitutes, arrFontSubstitutes);

   WritePrivateProfileString(NULL, NULL, NULL, m_szFileName);

   m_bDelayedInit = true;

   //強制フォント
   LPCTSTR lpszFace = GetForceFontName();
   if (lpszFace)
      g_pFTEngine->AddFont(lpszFace, FW_NORMAL, false);
}

bool CGdippSettings::LoadSettings()
{
   CCriticalSectionLock __lock;
   Assert(m_szFileName[0]);
   return LoadAppSettings(m_szFileName);
}

bool CGdippSettings::LoadSettings(HINSTANCE hModule)
{
   CCriticalSectionLock __lock;
   if (!::GetModuleFileName(hModule, m_szFileName, MAX_PATH - sizeof(".ini") + 1)) {
      return false;
   }

   LPTSTR p;
   if (p = StrStrI(m_szFileName, _T("-x86.dll")))
      *p = L'\0';
   else if (p = StrStrI(m_szFileName, _T("-x64.dll")))
      *p = L'\0';
   else
      *PathFindExtension(m_szFileName) = L'\0';

   StringCchCat(m_szFileName, MAX_PATH, _T(".ini"));
   return LoadAppSettings(m_szFileName);
}

int CGdippSettings::_GetFreeTypeProfileInt(LPCTSTR lpszKey, int nDefault, LPCTSTR lpszFile)
{
   const int retA = GetPrivateProfileInt(c_szFreeType, lpszKey, -1, lpszFile);
   const int retB = GetPrivateProfileInt(c_szFreeType, lpszKey, -2, lpszFile);

   if (retA == retB) {
      return retA;
   }
   return GetPrivateProfileInt(c_szGeneral, lpszKey, nDefault, lpszFile);
}

int CGdippSettings::_GetFreeTypeProfileBoundInt(LPCTSTR lpszKey, int nDefault, int nMin, int nMax, LPCTSTR lpszFile)
{
   const int ret = _GetFreeTypeProfileInt(lpszKey, nDefault, lpszFile);
   return Bound(ret, nMin, nMax);
}

float CGdippSettings::_GetFreeTypeProfileFloat(LPCTSTR lpszKey, float fDefault, LPCTSTR lpszFile)
{
   TCHAR TEMP[257];
   const int retA = GetPrivateProfileInt(c_szFreeType, lpszKey, -1, lpszFile);
   const int retB = GetPrivateProfileInt(c_szFreeType, lpszKey, -2, lpszFile);

   if (retA == retB) {
      GetPrivateProfileString(c_szFreeType, lpszKey, _T(""), TEMP, 256, lpszFile);
      return _StrToFloat(TEMP, fDefault);
   }
      GetPrivateProfileString(c_szGeneral, lpszKey, _T(""), TEMP, 256, lpszFile);
      return _StrToFloat(TEMP, fDefault);
}

float CGdippSettings::_GetFreeTypeProfileBoundFloat(LPCTSTR lpszKey, float fDefault, float fMin, float fMax, LPCTSTR lpszFile)
{
   const float ret = _GetFreeTypeProfileFloat(lpszKey, fDefault, lpszFile);
   return Bound(ret, fMin, fMax);
}


DWORD CGdippSettings::_GetFreeTypeProfileString(LPCTSTR lpszKey, LPCTSTR lpszDefault, LPTSTR lpszRet, DWORD cch, LPCTSTR lpszFile)
{
   const int retA = GetPrivateProfileInt(c_szFreeType, lpszKey, -1, lpszFile);
   const int retB = GetPrivateProfileInt(c_szFreeType, lpszKey, -2, lpszFile);

   if (retA == retB) {
      return GetPrivateProfileString(c_szFreeType, lpszKey, lpszDefault, lpszRet, cch, lpszFile);
   }
   return GetPrivateProfileString(c_szGeneral, lpszKey, lpszDefault, lpszRet, cch, lpszFile);
}

bool CGdippSettings::LoadAppSettings(LPCTSTR lpszFile)
{
   // 各種設定読み込み
   // INIファイルの例:
   // [General]
   // HookChildProcesses=0
   // HintingMode=0
   // AntiAliasMode=0
   // NormalWeight=0
   // BoldWeight=0
   // ItalicSlant=0
   // EnableKerning=0
   // MaxHeight=0
   // ForceChangeFont=ＭＳ Ｐゴシック
   // TextTuning=0
   // TextTuningR=0
   // TextTuningG=0
   // TextTuningB=0
   // CacheMaxFaces=0
   // CacheMaxSizes=0
   // CacheMaxBytes=0
   // AlternativeFile=
   // LoadOnDemand=0
   // UseMapping=0
   // LcdFilter=0
   // Shadow=1,1,4
   // [Individual]
   // ＭＳ Ｐゴシック=0,1,2,3,4,5

   WritePrivateProfileString(NULL, NULL, NULL, lpszFile);

   TCHAR szAlternative[MAX_PATH];
   if (GetPrivateProfileString(c_szGeneral, _T("AlternativeFile"), _T(""), szAlternative, MAX_PATH, lpszFile)) {
      if (PathIsRelative(szAlternative)) {
         TCHAR szDir[MAX_PATH];
         StringCchCopy(szDir, MAX_PATH, lpszFile);
         PathRemoveFileSpec(szDir);
         PathCombine(szAlternative, szDir, szAlternative);
      }
      StringCchCopy(m_szFileName, MAX_PATH, szAlternative);
      lpszFile = m_szFileName;
      WritePrivateProfileString(NULL, NULL, NULL, lpszFile);
   }

   CFontSettings& fs = m_FontSettings;
   fs.Clear();
   fs.SetHintingMode(_GetFreeTypeProfileBoundInt(_T("HintingMode"), 0, HINTING_MIN, HINTING_MAX, lpszFile));
   fs.SetAntiAliasMode(_GetFreeTypeProfileBoundInt(_T("AntiAliasMode"), 0, AAMODE_MIN, AAMODE_MAX, lpszFile));
   fs.SetNormalWeight(_GetFreeTypeProfileBoundInt(_T("NormalWeight"), 0, NWEIGHT_MIN, NWEIGHT_MAX, lpszFile));
   fs.SetBoldWeight(_GetFreeTypeProfileBoundInt(_T("BoldWeight"), 0, BWEIGHT_MIN, BWEIGHT_MAX, lpszFile));
   fs.SetItalicSlant(_GetFreeTypeProfileBoundInt(_T("ItalicSlant"), 0, SLANT_MIN, SLANT_MAX, lpszFile));
   fs.SetKerning(!!_GetFreeTypeProfileInt(_T("EnableKerning"), 0, lpszFile));

   {
      TCHAR szShadow[256];
      CStringTokenizer token;
      m_bEnableShadow = false;
      if (!_GetFreeTypeProfileString(_T("Shadow"), _T(""), szShadow, countof(szShadow), lpszFile)
            || token.Parse(szShadow) != 3) {
         goto SKIP;
      }
      for (int i=0; i<3; i++) {
         m_nShadow[i] = _StrToInt(token.GetArgument(i), 0);
         if (m_nShadow[i] <= 0) {
            goto SKIP;
         }
      }
      m_bEnableShadow = true;

SKIP:
      ;
   }

   m_bHookChildProcesses = !!GetPrivateProfileInt(c_szGeneral, _T("HookChildProcesses"), false, lpszFile);
   m_bUseMapping  = !!_GetFreeTypeProfileInt(_T("UseMapping"), false, lpszFile);
   m_nBolderMode  = _GetFreeTypeProfileInt(_T("BolderMode"), 0, lpszFile);
   m_nGammaMode   = _GetFreeTypeProfileInt(_T("GammaMode"), -1, lpszFile);
   m_fGammaValue  = _GetFreeTypeProfileBoundFloat(_T("GammaValue"), 1.0f, GAMMAVALUE_MIN, GAMMAVALUE_MAX, lpszFile);
   m_fRenderWeight   = _GetFreeTypeProfileBoundFloat(_T("RenderWeight"), 1.0f, RENDERWEIGHT_MIN, RENDERWEIGHT_MAX, lpszFile);
   m_fContrast    = _GetFreeTypeProfileBoundFloat(_T("Contrast"), 1.0f, CONTRAST_MIN, CONTRAST_MAX, lpszFile);
#ifdef _DEBUG
   // GammaValue検証用
   //CHAR GammaValueTest[1025];
   //sprintf(GammaValueTest, "GammaValue=%.6f\nContrast=%.6f\n", m_fGammaValue, m_fContrast);
   //MessageBoxA(NULL, GammaValueTest, "GammaValueテスト", 0);
#endif
   m_bLoadOnDemand   = !!_GetFreeTypeProfileInt(_T("LoadOnDemand"), false, lpszFile);
   m_bFontLink    = !!_GetFreeTypeProfileInt(_T("FontLink"), false, lpszFile);
   m_bIsInclude   = !!_GetFreeTypeProfileInt(_T("UseInclude"), false, lpszFile);
   m_nMaxHeight   = _GetFreeTypeProfileBoundInt(_T("MaxHeight"), 0, 0, 0x7fffffff, lpszFile);
   m_nLcdFilter   = _GetFreeTypeProfileInt(_T("LcdFilter"), 0, lpszFile);
   m_nFontSubstitutes = _GetFreeTypeProfileBoundInt(_T("FontSubstitutes"),
                                        SETTING_FONTSUBSTITUTE_DISABLE,
                                        SETTING_FONTSUBSTITUTE_DISABLE,
                                        SETTING_FONTSUBSTITUTE_ALL,
                                        lpszFile);
   m_nWidthMode = _GetFreeTypeProfileBoundInt(_T("WidthMode"),
                                    SETTING_WIDTHMODE_GDI32,
                                    SETTING_WIDTHMODE_GDI32,
                                    SETTING_WIDTHMODE_FREETYPE,
                                    lpszFile);
   m_nFontLoader = _GetFreeTypeProfileBoundInt(_T("FontLoader"),
                                    SETTING_FONTLOADER_FREETYPE,
                                    SETTING_FONTLOADER_FREETYPE,
                                    SETTING_FONTLOADER_WIN32,
                                    lpszFile);
   m_nCacheMaxFaces = _GetFreeTypeProfileInt(_T("CacheMaxFaces"), 0, lpszFile);
   m_nCacheMaxSizes = _GetFreeTypeProfileInt(_T("CacheMaxSizes"), 0, lpszFile);
   m_nCacheMaxBytes = _GetFreeTypeProfileInt(_T("CacheMaxBytes"), 0, lpszFile);

   if (m_nFontLoader == SETTING_FONTLOADER_WIN32) {
      // APIが処理してくれるはずなので自前処理は無効化
      if (m_nFontSubstitutes == SETTING_FONTSUBSTITUTE_ALL) {
         m_nFontSubstitutes = SETTING_FONTSUBSTITUTE_DISABLE;
      }
      m_bFontLink = FALSE;
   }

   // フォント指定
   ZeroMemory(&m_lfForceFont, sizeof(LOGFONT));
   m_szForceChangeFont[0] = _T('\0');
   _GetFreeTypeProfileString(_T("ForceChangeFont"), _T(""), m_szForceChangeFont, LF_FACESIZE, lpszFile);

   const int nTextTuning   = _GetFreeTypeProfileInt(_T("TextTuning"),  0, lpszFile),
           nTextTuningR = _GetFreeTypeProfileInt(_T("TextTuningR"), 0, lpszFile),
           nTextTuningG = _GetFreeTypeProfileInt(_T("TextTuningG"), 0, lpszFile),
           nTextTuningB = _GetFreeTypeProfileInt(_T("TextTuningB"), 0, lpszFile);

   InitInitTuneTable();
   InitTuneTable(nTextTuning,  m_nTuneTable);
   InitTuneTable(nTextTuningR, m_nTuneTableR);
   InitTuneTable(nTextTuningG, m_nTuneTableG);
   InitTuneTable(nTextTuningB, m_nTuneTableB);

   STARTUPINFO si = { sizeof(STARTUPINFO) };
   GetStartupInfo(&si);
   m_bRunFromGdiExe = IsGdiPPStartupInfo(si);
// if (!m_bRunFromGdiExe) {
//    m_bHookChildProcesses = false;
// }

// m_bIsHDBench = (GetModuleHandle(_T("HDBENCH.EXE")) == GetModuleHandle(NULL));

   m_arrExcludeFont.RemoveAll();
   m_arrExcludeModule.RemoveAll();
   m_arrIncludeModule.RemoveAll();

   // [Exclude]セクションから除外フォントリストを読み込む
   AddListFromSection(_T("Exclude"), lpszFile, m_arrExcludeFont);
   // [ExcludeModule]セクションから除外モジュールリストを読み込む
   AddListFromSection(_T("ExcludeModule"), lpszFile, m_arrExcludeModule);
   // [IncludeModule]セクションから対象モジュールリストを読み込む
   AddListFromSection(_T("IncludeModule"), lpszFile, m_arrIncludeModule);

   // [Individual]セクションからフォント別設定を読み込む
   AddIndividualFromSection(_T("Individual"), lpszFile, m_arrIndividual);

   WritePrivateProfileString(NULL, NULL, NULL, lpszFile);
   return true;
}

int CALLBACK CGdippSettings::EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* /*lptm*/, DWORD FontType, LPARAM lParam)
{
   CGdippSettings* pThis = reinterpret_cast<CGdippSettings*>(lParam);
   if (pThis && FontType == TRUETYPE_FONTTYPE)
      pThis->m_lfForceFont = *lplf;
   return 0;
}

template <class T>
bool CGdippSettings::AddListFromSection(LPCTSTR lpszSection, LPCTSTR lpszFile, CArray<T>& arr)
{
   LPTSTR  buffer = _GetPrivateProfileSection(lpszSection, lpszFile);
   if (buffer == NULL) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return false;
   }

   LPTSTR p = buffer;
   while (*p) {
      bool b = false;
      T str(p);
      for (int i = 0 ; i < arr.GetSize(); i++) {
         if (arr[i] == str) {
            b = true;
            break;
         }
      }
      if (!b) {
         arr.Add(str);
      }

      for (; *p; p++);
      p++;
   }
   delete[] buffer;
   return false;
}

bool CGdippSettings::AddIndividualFromSection(LPCTSTR lpszSection, LPCTSTR lpszFile, IndividualArray& arr)
{
   LPTSTR  buffer = _GetPrivateProfileSection(lpszSection, lpszFile);
   if (buffer == NULL) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return false;
   }

   LPTSTR p = buffer;
   while (*p) {
      bool b = false;

      LPTSTR pnext = p;
      for (; *pnext; pnext++);

      //"ＭＳ Ｐゴシック=0,0" みたいな文字列を分割
      LPTSTR value = _tcschr(p, _T('='));
      CStringTokenizer token;
      int argc = 0;
      if (value) {
         *value++ = _T('\0');
         argc = token.Parse(value);
      }

      CFontIndividual fi(p);
      const CFontSettings& fsCommon = m_FontSettings;
      CFontSettings& fs = fi.GetIndividual();
      //Individualが無ければ共通設定を使う
      fs = fsCommon;
      for (int i = 0; i < MAX_FONT_SETTINGS; i++) {
         LPCTSTR arg = token.GetArgument(i);
         if (!arg)
            break;
         const int n = _StrToInt(arg, fsCommon.GetParam(i));
         fs.SetParam(i, n);
      }

      for (int i = 0 ; i < arr.GetSize(); i++) {
         if (arr[i] == fi) {
            b = true;
            break;
         }
      }
      if (!b) {
         arr.Add(fi);
      }

      p = pnext;
      p++;
   }
   delete[] buffer;
   return false;
}

LPTSTR CGdippSettings::_GetPrivateProfileSection(LPCTSTR lpszSection, LPCTSTR lpszFile)
{
   LPTSTR buffer = NULL;
   int nRes = 0;
   for (int cch = 256; ; cch += 256) {
      buffer = new TCHAR[cch];
      if (!buffer) {
         break;
      }

      ZeroMemory(buffer, sizeof(TCHAR) * cch);
      nRes = GetPrivateProfileSection(lpszSection, buffer, cch, lpszFile);
      if (nRes < cch - 2) {
         break;
      }
      delete[] buffer;
      buffer = NULL;
   }
   return buffer;
}

//atolにデフォルト値を返せるようにしたような物
int CGdippSettings::_StrToInt(LPCTSTR pStr, int nDefault)
{
#define isspace(ch)     (ch == _T('\t') || ch == _T(' '))
#define isdigit(ch)     ((_TUCHAR)(ch - _T('0')) <= 9)

   int ret;
   bool neg = false;
   LPCTSTR pStart;

   for (; isspace(*pStr); pStr++);
   switch (*pStr) {
   case _T('-'):
      neg = true;
   case _T('+'):
      pStr++;
      break;
   }

   pStart = pStr;
   ret = 0;
   for (; isdigit(*pStr); pStr++) {
      ret = 10 * ret + (*pStr - _T('0'));
   }

   if (pStr == pStart) {
      return nDefault;
   }
   return neg ? -ret : ret;

#undef isspace
#undef isdigit
}

//atofにデフォルト値を返せるようにしたような物
float CGdippSettings::_StrToFloat(LPCTSTR pStr, float fDefault)
{
#define isspace(ch)     (ch == _T('\t') || ch == _T(' '))
#define isdigit(ch)     ((_TUCHAR)(ch - _T('0')) <= 9)

   int ret_i;
   int ret_d;
   float ret;
   bool neg = false;
   LPCTSTR pStart;

   for (; isspace(*pStr); pStr++);
   switch (*pStr) {
   case _T('-'):
      neg = true;
   case _T('+'):
      pStr++;
      break;
   }

   pStart = pStr;
   ret = 0;
   ret_i = 0;
   ret_d = 1;
   for (; isdigit(*pStr); pStr++) {
      ret_i = 10 * ret_i + (*pStr - _T('0'));
   }
   if (*pStr == _T('.')) {
      pStr++;
      for (; isdigit(*pStr); pStr++) {
         ret_i = 10 * ret_i + (*pStr - _T('0'));
         ret_d *= 10;
      }
   }
   ret = (float)ret_i / (float)ret_d;

   if (pStr == pStart) {
      return fDefault;
   }
   return neg ? -ret : ret;

#undef isspace
#undef isdigit
}

bool CGdippSettings::IsFontExcluded(LPCSTR lpFaceName) const
{
   WCHAR szStack[LF_FACESIZE];
   LPWSTR lpUnicode = _StrDupExAtoW(lpFaceName, -1, szStack, LF_FACESIZE, NULL);
   if (!lpUnicode) {
      return false;
   }

   bool b = IsFontExcluded(lpUnicode);
   if (lpUnicode != szStack)
      free(lpUnicode);
   return b;
}

bool CGdippSettings::IsFontExcluded(LPCWSTR lpFaceName) const
{
   StringHashFont* p = m_arrExcludeFont.Begin();
   StringHashFont* end = m_arrExcludeFont.End();
   StringHashFont str(lpFaceName);

   for(; p != end; ++p) {
      if (*p == str) {
         return true;
      }
   }
   return false;
}

bool CGdippSettings::IsProcessExcluded() const
{
   if (m_bRunFromGdiExe) {
      return false;
   }
   StringHashModule* p = m_arrExcludeModule.Begin();
   StringHashModule* end = m_arrExcludeModule.End();
   for(; p != end; ++p) {
      if (GetModuleHandleW(p->c_str())) {
         return true;
      }
   }
   return false;
}

bool CGdippSettings::IsProcessIncluded() const
{
   if (m_bRunFromGdiExe) {
      return true;
   }
   StringHashModule* p = m_arrIncludeModule.Begin();
   StringHashModule* end = m_arrIncludeModule.End();
   for(; p != end; ++p) {
      if (GetModuleHandleW(p->c_str())) {
         return true;
      }
   }
   return false;
}

void CGdippSettings::InitInitTuneTable()
{
   int i, *table;
#define init_table(name) \
      for (i=0,table=name; i<256; i++) table[i] = i
   init_table(m_nTuneTable);
   init_table(m_nTuneTableR);
   init_table(m_nTuneTableG);
   init_table(m_nTuneTableB);
#undef init_table
}

// テーブル初期化関数 0 - 12まで
// LCD用テーブル初期化関数 各0 - 12まで
void CGdippSettings::InitTuneTable(int v, int* table)
{
   int i;
   int col;
   double tmp, p;

   if (v < 0) {
      return;
   }
   v = Min(v, 12);
   p = (double)v;
   p = 1 - (p / (p + 10.0));
   for(i = 0;i < 256;i++){
       tmp = (double)i / 255.0;
        tmp = pow(tmp, p);
       col = 255 - (int)(tmp * 255.0 + 0.5);
      table[255 - i] = col;
   }
}

//見つからない場合は共通設定を返す
const CFontSettings& CGdippSettings::FindIndividual(LPCTSTR lpFaceName) const
{
   CFontIndividual* p      = m_arrIndividual.Begin();
   CFontIndividual* end = m_arrIndividual.End();
   StringHashFont hash(lpFaceName);

   for(; p != end; ++p) {
      if (p->GetHash() == hash) {
         return p->GetIndividual();
      }
   }
   return GetFontSettings();
}

bool CGdippSettings::CopyForceFont(LOGFONT& lf, const LOGFONT& lfOrg) const
{
   _ASSERTE(m_bDelayedInit);

   //&lf == &lfOrgも可
   bool bForceFont = !!GetForceFontName();
   const LOGFONT *lplf;
   if (bForceFont) {
      lplf = &m_lfForceFont;
   } else {
      lplf = GetFontSubstitutesInfo().lookup(lfOrg);
      if (lplf) bForceFont = true;
   }
   if (bForceFont) {
      const LOGFONT& lfff = *lplf;
      lf.lfHeight       = lfOrg.lfHeight;
//       lf.lfWidth        = lfOrg.lfWidth;
      lf.lfWidth        = 0;
      lf.lfEscapement      = lfOrg.lfEscapement;
      lf.lfOrientation  = lfOrg.lfOrientation;
      lf.lfWeight       = lfOrg.lfWeight;
      lf.lfItalic       = lfOrg.lfItalic;
      lf.lfUnderline    = lfOrg.lfUnderline;
      lf.lfStrikeOut    = lfOrg.lfStrikeOut;
      lf.lfCharSet      = lfff.lfCharSet;
      lf.lfOutPrecision = 0;
      lf.lfClipPrecision   = 0;
      lf.lfQuality      = 0;
      lf.lfPitchAndFamily  = 0;
      StringCchCopy(lf.lfFaceName, LF_FACESIZE, lfff.lfFaceName);
   }
   return bForceFont;
}

//値的にchar(-128～127)で十分
const char CFontSettings::m_bound[MAX_FONT_SETTINGS][2] = {
   { HINTING_MIN, HINTING_MAX }, //Hinting
   { AAMODE_MIN,  AAMODE_MAX  }, //AAMode
   { NWEIGHT_MIN, NWEIGHT_MAX }, //NormalWeight
   { BWEIGHT_MIN, BWEIGHT_MAX }, //BoldWeight
   { SLANT_MIN,   SLANT_MAX   }, //ItalicSlant
   { 0,        1        }, //Kerning
};

CFontLinkInfo::CFontLinkInfo()
{
   memset(&info, 0, sizeof info);
}

CFontLinkInfo::~CFontLinkInfo()
{
   clear();
}

void CFontLinkInfo::init()
{
   /* get default gui font: MS Shell Dlg */

   /*
      FIXME: Default FontLink stored in MS Shell Dlg registry key (maybe it's Microsoft Sans 
             Serif as I know) is only available under English and Japanese locales. It should
             not be introduced in Chinese/PRC and other locales, as it cause more problem than
             it's benefit.
             
             I should determine locale here. But let's silently discard DEFAULT_GUI_FONT for now.
    */
#if 0
   HGDIOBJ h = GetStockObject(DEFAULT_GUI_FONT);
   GetObject(h, sizeof syslf, &syslf);
#endif
   default_font = NULL;

   /* open registry keys */
   const WCHAR rkFontLink[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink";
   const WCHAR rkFontName[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
   HKEY hkFontLink = NULL, hkFontName = NULL;
   if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, rkFontLink, 0, KEY_QUERY_VALUE, &hkFontLink))
      goto init_error;
   if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_LOCAL_MACHINE, rkFontName, 0, KEY_QUERY_VALUE, &hkFontName))
      goto init_error;

#define STRMAX 0x2000

   int i = 0, row = 0;
   while (row < INFOMAX) {
      WCHAR fname[STRMAX], flink[STRMAX];
      DWORD fname_len = STRMAX, flink_len = STRMAX * sizeof(WCHAR);
      DWORD regtype;

      /* get next font link */
      LONG rc = RegEnumValueW(hkFontLink, i++, fname, &fname_len, 0, &regtype, (LPBYTE)flink, &flink_len);
      if (rc == ERROR_NO_MORE_ITEMS || rc != ERROR_SUCCESS)
         break;
      if (regtype != REG_MULTI_SZ)
         continue;

      int col = 0;
      info[row][col++] = _wcsdup(fname);

      LPCWSTR linep = flink;
      /* enum all lines in REG_MULTI_SZ */
      for (LPCWSTR linep = flink; col < FONTMAX && *linep; linep += wcslen(linep) + 1) {
         WCHAR linkfont[STRMAX];
         WCHAR linkfile[STRMAX];
         StringCchCopyW(linkfile, STRMAX, linep);

         LPTSTR p = wcsrchr(linkfile, L','); /* find last ',' */
         if (p && iswdigit(*(p + 1))) /* ',' in fontlink, and 1-9 after ',' */
            continue;
         if (p && !iswdigit(*(p + 1))) /* ',' in fontlink, and no digit after ',' */
            StringCchCopyW(linkfont, STRMAX, p + 1);
         
         if (!p) { /* no ',' in fontlink, check fontname in hkFontName */
            linkfont[0] = L'\0';
            for(int j = 0; ; ++j) {
               WCHAR name[STRMAX], file[STRMAX];
               DWORD name_len = STRMAX, file_len = STRMAX * sizeof(WCHAR);
               DWORD regtype;

               LONG rc = RegEnumValueW(hkFontName, j, name, &name_len, 0, &regtype, (LPBYTE)file, &file_len);
               if (rc == ERROR_NO_MORE_ITEMS || rc != ERROR_SUCCESS)
                  break;
               if (regtype != REG_SZ)
                  continue;
               if (StrCmpIW(file, linkfile) != 0) 
                  continue;
               StringCchCopyW(linkfont, STRMAX, name);
               break;
            }

            if (linkfont[0] == L'\0') /* font not registered, ignore */
               continue;
            
            if (p = StrStrIW(linkfont, L" (")) /* remove (Truetype) */
               *p = L'\0';
            if (p = StrStrIW(linkfont, L" &")) /* remove & OtherName */
               *p = L'\0';
         }
         info[row][col++] = _wcsdup(linkfont);
      }

      if (col == 1) {
         free(info[row][0]);
         info[row][0] = NULL;
      }
      else {
         CString key = info[row][0];
         key.MakeLower();         
         index[key] = row;

         /* If FontLink is Tahoma, then save the first item as default font that all font links
            to. This is the default behavior under CJK locale (I assume), although gdi32.dll
            hardcoded this font. But I use this trick to get default font in CJK locale */
         if (key.CompareNoCase(L"Tahoma") == 0) {
            Assert(info[row][1]);
            default_font = info[row][1];
         }
         ++row;
      }
   }

#undef STRMAX
   
   /* copy default font link */
   const LPCWSTR *p_default = this->lookup(sysfn());
   if (p_default) {
      for (int i = 0; info[i][0]; ++i) {
         if (lstrcmpiW(info[i][0], sysfn()) == 0)
            continue;
         int end = 0;
         for (; info[i][end]; ++end);
         for (int j = 0; p_default[j]; ++j)
            info[i][end + j] = _wcsdup(p_default[j]);
      }
   }

   /* free resources */
init_error:
   if (hkFontLink)
      RegCloseKey(hkFontLink);
   if (hkFontName)
      RegCloseKey(hkFontName);
}

void CFontLinkInfo::clear()
{
   for (int i = 0; i < INFOMAX; ++i) {
      for (int j = 0; j < FONTMAX; ++j) {
         free(info[i][j]);
         info[i][j] = NULL;
      }
   }
   index.RemoveAll();
}

const LPCWSTR * CFontLinkInfo::lookup(LPCWSTR fontname) const
{
   CString key(fontname);
   key.MakeLower();
   int row;
   if (index.Lookup(key, row))
      return &info[row][1];
   return NULL;
}

CFontSubstituteData::CFontSubstituteData()
{
   memset(this, 0, sizeof *this);
}

int CALLBACK
CFontSubstituteData::EnumFontFamProc(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD /*FontType*/, LPARAM lParam)
{
   CFontSubstituteData& self = *(CFontSubstituteData *)lParam;
   self.m_lf = *lplf;
   self.m_tm = *lptm;
   return 0;
}

bool
CFontSubstituteData::init(LPCTSTR config)
{
   memset(this, 0, sizeof *this);

   TCHAR buf[LF_FACESIZE + 20];
   StringCchCopy(buf, countof(buf), config);

   LOGFONT lf;
   memset(&lf, 0, sizeof lf);

   LPTSTR p;
   for (p = buf + lstrlen(buf) - 1; p >= buf; --p ) {
      if (*p == _T(',')) {
         *p++ = 0;
         break;
      }
   }
   if (p >= buf) {
      StringCchCopy(lf.lfFaceName, countof(lf.lfFaceName), buf);
      lf.lfCharSet = (BYTE)CGdippSettings::_StrToInt(p + 1, 0);
      m_bCharSet = true;
   } else {
      StringCchCopy(lf.lfFaceName, LF_FACESIZE, buf);
      lf.lfCharSet = DEFAULT_CHARSET;
      m_bCharSet = false;
   }

   HDC hdc = GetDC(NULL);
   EnumFontFamiliesEx(hdc, &lf, &CFontSubstituteData::EnumFontFamProc, (LPARAM)this, 0);
   ReleaseDC(NULL, hdc);

   return m_lf.lfFaceName[0] != 0;
}

bool
CFontSubstituteData::operator == (const CFontSubstituteData& o) const
{
   if (m_bCharSet != o.m_bCharSet) return false;
   if (m_bCharSet) {
      if (m_lf.lfCharSet != o.m_lf.lfCharSet) return false;
   }
   if (lstrcmpi(m_lf.lfFaceName, o.m_lf.lfFaceName) != 0) return false;
   return true;
}


void
CFontSubstitutesInfo::initreg()
{
   const LPCTSTR REGKEY = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");
   HKEY h;
   if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY, 0, KEY_QUERY_VALUE, &h)) return;

   for (int i = 0; ; ++i) {
      WCHAR name[0x2000];
      DWORD namesz;
      WCHAR value[0x2000];
      DWORD valuesz;
      namesz = sizeof name;
      valuesz = sizeof value;
      DWORD regtype;

      LONG rc = RegEnumValue(h, i, name, &namesz, 0, &regtype, (LPBYTE)value, &valuesz);
      if (rc == ERROR_NO_MORE_ITEMS) break;
      if (rc != ERROR_SUCCESS) break;
      if (regtype != REG_SZ) continue;

      CFontSubstituteData k;
      CFontSubstituteData v;
      if (k.init(name) && v.init(value)) {
         if (FindKey(k) < 0 && k.m_bCharSet == v.m_bCharSet) Add(k, v);
      }
   }

   RegCloseKey(h);
}

void
CFontSubstitutesInfo::initini(const CFontSubstitutesIniArray& iniarray)
{
   for (int i = 0; i < iniarray.GetSize(); ++i) {
      LPCTSTR inistr = iniarray[i].c_str();
      LPTSTR buf = _tcsdup(inistr);
      for (LPTSTR vp = buf; *vp; ++vp) {
         if (*vp == _T('=')) {
            *vp++ = 0;
            CFontSubstituteData k;
            CFontSubstituteData v;
            if (k.init(buf) && v.init(vp)) {
            if (FindKey(k) < 0 && k.m_bCharSet == v.m_bCharSet) Add(k, v);
            }
         }
      }

      free(buf);
   }
}

void
CFontSubstitutesInfo::init(int nFontSubstitutes, const CFontSubstitutesIniArray& iniarray)
{
   if (nFontSubstitutes >= SETTING_FONTSUBSTITUTE_INIONLY) initini(iniarray);
   if (nFontSubstitutes >= SETTING_FONTSUBSTITUTE_ALL) initreg();
}

const LOGFONT *
CFontSubstitutesInfo::lookup(const LOGFONT& lf) const
{
   if (GetSize() <= 0) return false;

   CFontSubstituteData v;
   CFontSubstituteData k;
   StringCchCopy(k.m_lf.lfFaceName, LF_FACESIZE, lf.lfFaceName);

   k.m_bCharSet = true;
   k.m_lf = lf;
   int pos = FindKey(k);
   if (pos < 0) {
      k.m_bCharSet = false;
      pos = FindKey(k);
   }
   if (pos >= 0) {
      return (const LOGFONT *)&GetValueAt(pos);
   }
   return NULL;
}

CFontFaceNamesEnumerator::CFontFaceNamesEnumerator(LPCWSTR facename)
{
   const CFontLinkInfo& info = CGdippSettings::GetInstance()->GetFontLinkInfo();
   p_facename = info.lookup(facename);
   if (p_facename)
      return;

#if 0 /* discard MS Shell Dlg for now */
   p_facename = info.lookup(info.sysfn());
   if (p_facename)
      return;
#endif

   m_facenames[0] = info.sysfn();
   m_facenames[1] = NULL;
   p_facename = m_facenames;
   return;
}
