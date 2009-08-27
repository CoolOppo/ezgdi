#pragma once

#include "common.h"
#include "cache.h"


#define MAX_FONT_SETTINGS	16
#define DEFINE_FS_MEMBER(name, param) \
	int  Get##name() const { return GetParam(param); } \
	void Set##name(int n)  { SetParam(param, n); }

class CFontSettings
{
private:
	int m_settings[MAX_FONT_SETTINGS];
	static const char m_bound[MAX_FONT_SETTINGS][2];

	enum _FontSettingsParams {
		FSP_HINTING			= 0,
		FSP_AAMODE			= 1,
		FSP_NORMAL_WEIGHT	= 2,
		FSP_BOLD_WEIGHT		= 3,
		FSP_ITALIC_SLANT	= 4,
		FSP_KERNING			= 5,
	};

public:
	CFontSettings()
	{
		Clear();
	}

	DEFINE_FS_MEMBER(HintingMode,	FSP_HINTING);
	DEFINE_FS_MEMBER(AntiAliasMode,	FSP_AAMODE);
	DEFINE_FS_MEMBER(NormalWeight,	FSP_NORMAL_WEIGHT);
	DEFINE_FS_MEMBER(BoldWeight,	FSP_BOLD_WEIGHT);
	DEFINE_FS_MEMBER(ItalicSlant,	FSP_ITALIC_SLANT);
	DEFINE_FS_MEMBER(Kerning,		FSP_KERNING);

	int GetParam(int x) const
	{
		Assert(0 <= x && x < MAX_FONT_SETTINGS);
		return m_settings[x];
	}
	void SetParam(int x, int n)
	{
		Assert(0 <= x && x < MAX_FONT_SETTINGS);
		m_settings[x] = Bound<int>(n, m_bound[x][0], m_bound[x][1]);
	}
	void Clear()
	{
		ZeroMemory(m_settings, sizeof(m_settings));
	}

	void SetSettings(const int* p, int count)
	{
		count = Min(count, MAX_FONT_SETTINGS);
		memcpy(m_settings, p, count * sizeof(int));
	}
};

#undef DEFINE_FS_MEMBER


class CFontIndividual
{
	CFontSettings	m_set;
	StringHashFont	m_hash;

public:
	CFontIndividual()
	{
	}
	CFontIndividual(LPCTSTR name)
		: m_hash(name)
	{
	}

	CFontSettings& GetIndividual() { return m_set; }
	LPCTSTR GetName() const { return m_hash.c_str(); }
	const StringHashFont& GetHash() const { return m_hash; }
	bool operator ==(const CFontIndividual& x) const { return (m_hash == x.m_hash); }
};

class CFontLinkInfo
{
public:
	enum {
		INFOMAX = 15,
		FONTMAX = 31,
	};
private:
	LPWSTR info[INFOMAX + 1][FONTMAX + 1];
	LOGFONT syslf;
public:
	CFontLinkInfo();
	~CFontLinkInfo();
	void init();
	void clear();
	const LPCWSTR sysfn() const { return syslf.lfFaceName; }
	const LPCWSTR * lookup(LPCWSTR fontname) const;
	LPCWSTR get(int row, int col) const;
};

class CFontSubstitutesInfo;

class CFontSubstituteData
{
	friend CFontSubstitutesInfo;
private:
	LOGFONT m_lf;
	TEXTMETRIC m_tm;
	bool m_bCharSet;
public:
	bool operator == (const CFontSubstituteData& o) const;
private:
	CFontSubstituteData();
	bool init(LPCTSTR config);
	static int CALLBACK EnumFontFamProc(const LOGFONT *lplf, const TEXTMETRIC *lptm, DWORD FontType, LPARAM lParam);

};

typedef StringHashT<LF_FACESIZE + 10,true> CFontSubstitutesHash;
typedef CArray<CFontSubstitutesHash> CFontSubstitutesIniArray;

class CFontSubstitutesInfo : public CSimpleMap<CFontSubstituteData, CFontSubstituteData>
{
private:
	void initini(const CFontSubstitutesIniArray& iniarray);
	void initreg();
public:
	void init(int nFontSubstitutes, const CFontSubstitutesIniArray& iniarray);
	const LOGFONT * lookup(const LOGFONT &lf) const;
};

#define SETTING_FONTSUBSTITUTE_DISABLE (0)
#define SETTING_FONTSUBSTITUTE_INIONLY (1)
#define SETTING_FONTSUBSTITUTE_ALL     (2)

#define SETTING_WIDTHMODE_GDI32    (0)
#define SETTING_WIDTHMODE_FREETYPE (1)

#define SETTING_FONTLOADER_FREETYPE  (0)
#define SETTING_FONTLOADER_WIN32     (1)

class CGdippSettings;

class CGdippSettings
{
	friend CFontSubstituteData;
	friend BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
private:
	static CGdippSettings* s_pInstance;
	//INI用
	CFontSettings m_FontSettings;
	bool m_bHookChildProcesses		: 1;
	bool m_bUseMapping				: 1;
	bool m_bLoadOnDemand			: 1;
	bool m_bEnableShadow			: 1;
	bool m_bFontLink				: 1;

	//それ以外
	bool m_bIsWinXPorLater			: 1;
	bool m_bRunFromGdiExe			: 1;
	bool m_bIsInclude				: 1;
	bool m_bDelayedInit				: 1;
//	bool m_bIsHDBench				: 1;
//	bool m_bHaveNewerFreeType		: 1;
	bool							: 0;

	int  m_nBolderMode;
	int  m_nGammaMode;
	float m_fGammaValue;
	float m_fRenderWeight;
	float m_fContrast;
	int  m_nMaxHeight;
	int  m_nLcdFilter;
	int  m_nShadow[3];
	int  m_nFontSubstitutes;
	int  m_nWidthMode;
	int  m_nFontLoader;

	//FTC_Manager_Newに渡すパラメータ
	int  m_nCacheMaxFaces;
	int  m_nCacheMaxSizes;
	int  m_nCacheMaxBytes;

	// アンチエイリアス調整用テーブル
	int  m_nTuneTable[256];
	// LCD用
	int  m_nTuneTableR[256];
	int  m_nTuneTableG[256];
	int  m_nTuneTableB[256];

	typedef CArray<StringHashFont>		FontHashArray;
	typedef CArray<StringHashModule>	ModuleHashArray;
	typedef CArray<CFontIndividual>		IndividualArray;
	FontHashArray	m_arrExcludeFont;
	ModuleHashArray	m_arrExcludeModule;
	ModuleHashArray	m_arrIncludeModule;
	IndividualArray	m_arrIndividual;

	// 指定フォント
	LOGFONT m_lfForceFont;
	TCHAR m_szForceChangeFont[LF_FACESIZE];

	//INIファイル名
	TCHAR m_szFileName[MAX_PATH];

	//INIからの読み込み処理
	bool LoadAppSettings(LPCTSTR lpszFile);
	static LPTSTR _GetPrivateProfileSection    (LPCTSTR lpszSection, LPCTSTR lpszFile);
	static int    _GetFreeTypeProfileInt       (LPCTSTR lpszKey, int nDefault, LPCTSTR lpszFile);
	static int    _GetFreeTypeProfileBoundInt  (LPCTSTR lpszKey, int nDefault, int nMin, int nMax, LPCTSTR lpszFile);
	static float  _GetFreeTypeProfileFloat     (LPCTSTR lpszKey, float fDefault, LPCTSTR lpszFile);
	static float  _GetFreeTypeProfileBoundFloat(LPCTSTR lpszKey, float fDefault, float fMin, float fMax, LPCTSTR lpszFile);
	static DWORD  _GetFreeTypeProfileString    (LPCTSTR lpszKey, LPCTSTR lpszDefault, LPTSTR lpszRet, DWORD cch, LPCTSTR lpszFile);
	static int CALLBACK EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam);

	template <class T>
	static bool AddListFromSection(LPCTSTR lpszSection, LPCTSTR lpszFile, CArray<T>& arr);
	bool AddIndividualFromSection(LPCTSTR lpszSection, LPCTSTR lpszFile, IndividualArray& arr);
	static int   _StrToInt(LPCTSTR pStr, int nDefault);
	static float _StrToFloat(LPCTSTR pStr, float fDefault);

	void InitInitTuneTable();
	static void InitTuneTable(int v, int* table);
	void DelayedInit();

	CFontLinkInfo m_fontlinkinfo;
	CFontSubstitutesInfo m_FontSubstitutesInfo;

	CGdippSettings()
		: m_bHookChildProcesses(false)
		, m_bUseMapping(false)
		, m_bLoadOnDemand(false)
		, m_bEnableShadow(false)
		, m_bFontLink(false)
//		, m_bEnableKerning(false)
		, m_bIsWinXPorLater(false)
		, m_bRunFromGdiExe(false)
		, m_bIsInclude(false)
		, m_bDelayedInit(false)
//		, m_bIsHDBench(false)
//		, m_bHaveNewerFreeType(false)
		, m_nBolderMode(0)
		, m_nGammaMode(0)
		, m_fGammaValue(1.0f)
		, m_fRenderWeight(1.0f)
		, m_fContrast(1.0f)
		, m_nMaxHeight(0)
		, m_nLcdFilter(0)
		, m_nCacheMaxFaces(0)
		, m_nCacheMaxSizes(0)
		, m_nCacheMaxBytes(0)
	{
		ZeroMemory(m_nTuneTable,		sizeof(m_nTuneTable));
		ZeroMemory(m_nTuneTableR,		sizeof(m_nTuneTableR));
		ZeroMemory(m_nTuneTableG,		sizeof(m_nTuneTableG));
		ZeroMemory(m_nTuneTableB,		sizeof(m_nTuneTableB));
		ZeroMemory(&m_lfForceFont,		sizeof(LOGFONT));
		ZeroMemory(m_nShadow,			sizeof(m_nShadow));
		ZeroMemory(m_szFileName,		sizeof(m_szFileName));
		ZeroMemory(m_szForceChangeFont,	sizeof(m_szForceChangeFont));
	}

	~CGdippSettings()
	{
	}

	static CGdippSettings* CreateInstance();
	static void DestroyInstance();

public:
	static const CGdippSettings* GetInstance();
	static const CGdippSettings* GetInstanceNoInit();	//FreeTypeFontEngine

	//INI用
	const CFontSettings& GetFontSettings() const { return m_FontSettings; }
	bool HookChildProcesses() const { return m_bHookChildProcesses; }
	bool UseMapping() const { return m_bUseMapping; }
	bool LoadOnDemand() const { return m_bLoadOnDemand; }
	bool FontLink() const { return m_bFontLink; }
//	bool EnableKerning() const { return m_bEnableKerning; }

	int BolderMode() const { return m_nBolderMode; }
	int GammaMode() const { return m_nGammaMode; }
	float GammaValue() const { return m_fGammaValue; }
	float RenderWeight() const { return m_fRenderWeight; }
	float Contrast() const { return m_fContrast; }
	int MaxHeight() const { return m_nMaxHeight; }
	int LcdFilter() const { return m_nLcdFilter; }
	int WidthMode() const { return m_nWidthMode; }
	int FontLoader() const { return m_nFontLoader; }
	int CacheMaxFaces() const { return m_nCacheMaxFaces; }
	int CacheMaxSizes() const { return m_nCacheMaxSizes; }
	int CacheMaxBytes() const { return m_nCacheMaxBytes; }

	bool EnableShadow()  const { return m_bEnableShadow; }
	const int* GetShadowParams() const { return m_nShadow; }

	// フォント名よみとり
	LPCTSTR GetForceFontName() const
	{
		_ASSERTE(m_bDelayedInit);
		LPCTSTR lpszFace = m_lfForceFont.lfFaceName;
		return lpszFace[0] ? lpszFace : NULL;
	}

	bool CopyForceFont(LOGFONT& lf, const LOGFONT& lfOrg) const;

	//それ以外
	bool IsWinXPorLater() const { return m_bIsWinXPorLater; }
	bool IsInclude() const { return m_bIsInclude; }
//	bool IsHDBench() const { return m_bIsHDBench; }
	bool RunFromGdiExe() const { return m_bRunFromGdiExe; }
//	bool HaveNewerFreeType() const { return m_bHaveNewerFreeType; }
	const int* GetTuneTable() const { return m_nTuneTable; }
	const int* GetTuneTableR() const { return m_nTuneTableR; }
	const int* GetTuneTableG() const { return m_nTuneTableG; }
	const int* GetTuneTableB() const { return m_nTuneTableB; }

	bool LoadSettings();
	bool LoadSettings(HINSTANCE hModule);

	bool IsFontExcluded(LPCSTR lpFaceName) const;
	bool IsFontExcluded(LPCWSTR lpFaceName) const;

	bool IsProcessExcluded() const;
	bool IsProcessIncluded() const;
	const CFontSettings& FindIndividual(LPCTSTR lpFaceName) const;

	const CFontLinkInfo& GetFontLinkInfo() const
		{ _ASSERTE(m_bDelayedInit); return m_fontlinkinfo; }
	const CFontSubstitutesInfo& GetFontSubstitutesInfo() const
		{ _ASSERTE(m_bDelayedInit); return m_FontSubstitutesInfo; }
};

class CFontFaceNamesEnumerator
{
private:
	enum {
		MAXFACENAMES = CFontLinkInfo::FONTMAX * 2 + 1,
	};
	LPCWSTR m_facenames[MAXFACENAMES];
	int m_pos;
	int m_endpos;
	CFontFaceNamesEnumerator();
public:
	CFontFaceNamesEnumerator(LPCWSTR facename);
	operator LPCWSTR () {
		return m_facenames[m_pos];
	}
	void next() {
		++m_pos;
	}
	bool atend() {
		return !!(m_pos >= m_endpos);
	}
};
