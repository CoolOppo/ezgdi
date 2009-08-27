#ifndef FT_H
#define FT_H

#define FTO_MONO		0x0001
#define FTO_SIZE_ONLY	0x0002


BOOL FontLInit(void);
void FontLFree(void);

struct FREETYPE_PARAMS
{
	UINT etoOptions;
	UINT ftOptions;
	int charExtra;
	COLORREF color;
	int alpha;
	const LOGFONTW* lplf;

	FREETYPE_PARAMS()
	{
		ZeroMemory(this, sizeof(*this));
	}

	//FreeTypeGetTextExtentPoint用 (サイズ計算)
	FREETYPE_PARAMS(UINT eto, HDC hdc)
		: etoOptions(eto)
		, ftOptions(FTO_SIZE_ONLY)
		, charExtra(GetTextCharacterExtra(hdc))
		, color(0)
		, alpha(0)
		, lplf(NULL)
	{
	}

	//FreeTypeTextOut用 (サイズ計算＋文字描画)
	FREETYPE_PARAMS(UINT eto, HDC hdc, LOGFONTW* p)
		: etoOptions(eto)
		, ftOptions(0)
		, charExtra(GetTextCharacterExtra(hdc))
		, color(GetTextColor(hdc))
		, alpha(0)
		, lplf(p)
	{
	}

	bool IsMono() const
	{
		return (ftOptions & FTO_MONO);
	}
};

BOOL FreeTypeTextOut(
	const HDC hdc,
	CBitmapCache& cache,
	const int nXStart,
	const int nYStart,
	LPCWSTR lpString,
	int cbString,
	const int* lpDx,
	const FREETYPE_PARAMS* params,
	int &width
	);
BOOL FreeTypeGetTextExtentPoint(
	const HDC hdc,
	LPCWSTR lpString,
	int cbString,
	LPSIZE lpSize,
	const FREETYPE_PARAMS* params
	);
BOOL FreeTypeGetCharWidth(
	const HDC hdc,
	UINT iFirstChar,
	UINT iLastChar,
	LPINT lpBuffer
	);
void FreeTypeSubstGlyph(
	const HDC hdc,
	const WORD vsindex,
	const int baseChar,
	int cChars, 
	SCRIPT_ANALYSIS* psa, 
	WORD* pwOutGlyphs, 
	WORD* pwLogClust, 
	SCRIPT_VISATTR* psva, 
	int* pcGlyphs 
	);
#endif
