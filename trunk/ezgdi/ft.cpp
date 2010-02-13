#include "stdafx.h"

#include "override.h"
#include "ft.h"
#include "fteng.h"
#include "ft2vert.h"

#define IsFontBold(lf)     ((lf).lfWeight > FW_NORMAL)
#define FT_FixedToInt(x)   (FT_RoundFix(x) >> 16)
#define FT_PosToInt(x)     (((x) + (1 << 5)) >> 6)
#define RESOLUTION_X 72
#define RESOLUTION_Y 72
FT_Error New_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos str_h, FT_Pos str_v );
FT_Error Old_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength );
FT_Error Vert_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength );

class CGGOKerning : public CMap<DWORD, int>
{
private:
   DWORD makekey(WORD first, WORD second) {
      return ((DWORD)first << 16) | second;
   }
public:
   void init(HDC hdc);
   int get(WORD first, WORD second) {
      DWORD key = makekey(first, second);
      int x = FindKey(key);
      return (x >= 0) ? GetValueAt(x) : 0;
   }
};

void
CGGOKerning::init(HDC hdc)
{
   DWORD rc;
   rc = GetKerningPairs(hdc, 0, NULL);
   if (rc <= 0) return;
   DWORD kpcnt = rc;
   LPKERNINGPAIR kpp = (LPKERNINGPAIR)calloc(kpcnt, sizeof *kpp);
   if (!kpp) return;
   rc = GetKerningPairs(hdc, kpcnt, kpp);
   for (DWORD i = 0; i < rc; ++i) {
      Add(makekey(kpp[i].wFirst, kpp[i].wSecond), kpp[i].iKernAmount);
   }
   free(kpp);
}

struct FreeTypeDrawInfo
{
   FT_FaceRec_ dummy_freetype_face;

   //FreeTypePrepareが設定する
   FT_Face freetype_face;
   FT_Int cmap_index;
   FT_Bool useKerning;
   FT_Render_Mode render_mode;
   FTC_ImageTypeRec font_type;
   FreeTypeFontInfo* pfi;
   const CFontSettings* pfs;
   FreeTypeFontCache* pftCache;
   FTC_FaceID face_id_list[CFontLinkInfo::FONTMAX * 2 + 1];
   int face_id_list_num;

   //呼び出し前に自分で設定する
   HDC hdc;
   int x;
   int yBase;
   int yTop;
   const int* lpDx;
   CBitmapCache* pCache;
   FREETYPE_PARAMS params;

   FreeTypeDrawInfo(const FREETYPE_PARAMS& fp, HDC dc, LOGFONTW* lf = NULL, CBitmapCache* ca = NULL, const int* dx = NULL)
      : freetype_face(&dummy_freetype_face), cmap_index(0), useKerning(0)
      , pfi(NULL), pfs(NULL), pftCache(NULL), face_id_list_num(0)
      , hdc(dc), x(0), yBase(0), yTop(0)
   {
      render_mode = FT_RENDER_MODE_NORMAL;
      ZeroMemory(&font_type, sizeof(font_type));
      ZeroMemory(&face_id_list, sizeof face_id_list);
      lpDx   = dx;
      pCache = ca;
      params = fp;
      if(lf) params.lplf = lf;
      memset(&dummy_freetype_face, 0, sizeof dummy_freetype_face);
   }

   const LOGFONTW& LogFont() const { return *params.lplf; }
   COLORREF Color() const { return params.color; }
   UINT GetETO() const { return params.etoOptions; }
   bool IsGlyphIndex() const { return !!(params.etoOptions & ETO_GLYPH_INDEX); }
   bool IsMono() const { return !!(params.ftOptions & FTO_MONO); }
   bool IsSizeOnly() const { return !!(params.ftOptions & FTO_SIZE_ONLY); }
   CGGOKerning ggokerning;
};

//fteng.cppで定義
extern FT_Library     freetype_library;
extern FTC_Manager    cache_man;
extern FTC_CMapCache  cmap_cache;
extern FTC_ImageCache image_cache;

class CAlphaBlend
{
private:
   int alphatbl[256];
   int tbl1[257];
   BYTE tbl2[256 * 16 + 1];
   // 通常のアルファ値補正
   int tunetbl[256];
   int tunetblR[256];
   int tunetblG[256];
   int tunetblB[256];
   // 影文字用のアルファ値補正
   int tunetblS[256];
   int tunetblRS[256];
   int tunetblGS[256];
   int tunetblBS[256];
public:
   static const int BASE;
public:
   CAlphaBlend() { }
   ~CAlphaBlend() {}
   void init();
   BYTE doAB(BYTE fg, BYTE bg, int alpha);
   void gettunetbl(int paramalpha, bool lcd, const int * &tblR, const int * &tblG, const int * &tblB) const;
   inline int conv1(BYTE n) {
      return tbl1[n];
   }
   inline BYTE conv2(int n) {
      return tbl2[n / (BASE * BASE / (sizeof tbl2 - 1))];
   }
private:
   inline int convalpha(int alpha) {
      return alphatbl[alpha];
   }
   inline BYTE rconv1(int n);
};
const int CAlphaBlend::BASE = 0x4000;

static CAlphaBlend s_AlphaBlendTable;

void CAlphaBlend::gettunetbl(int paramalpha, bool lcd, const int * &tblR, const int * &tblG, const int * &tblB) const
{
   if (paramalpha == 1) {
      if (lcd) {
         tblR = tunetblR;
         tblG = tunetblG;
         tblB = tunetblB;
      } else {
         tblR = tblG = tblB = tunetbl;
      }
   } else {
      if (lcd) {
         tblR = tunetblRS;
         tblG = tunetblGS;
         tblB = tunetblBS;
      } else {
         tblR = tblG = tblB = tunetblS;
      }
   }
}

void CAlphaBlend::init()
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   const float gamma = pSettings->GammaValue();
   const float weight = pSettings->RenderWeight();
   const float contrast = pSettings->Contrast();
   const int mode = pSettings->GammaMode();

   int i;
   float temp, alpha;

   for (i = 0; i < 256; ++i) {
      temp = pow((1.0f / 255.0f) * i, 1.0f / weight);

      if  (temp < 0.5f) {
         alpha = pow(temp * 2, contrast) / 2.0f;
      } else {
         alpha = 1.0f - pow((1.0f - temp) * 2, contrast) / 2.0f;
      }
      alphatbl[i] = (int)(alpha * BASE);

      if (mode < 0) {
         temp = (1.0f / 255.0f) * i;
      } else {
         if (mode == 1) {
            if (i <= 10) {
               temp = (float)i / (12.92f * 255.0f);
            } else {
               temp = pow(((1.0f / 255.0f) * i + 0.055f) / 1.055f, 2.4f);
            }
         } else if (mode == 2) {
            if (i <= 10) {
               temp = ((float)i / (12.92f * 255.0f) + (float)i / 255.0f) / 2;
            } else {
               temp = (pow(((1.0f / 255.0f) * i + 0.055f) / 1.055f, 2.4f) + (float)i / 255.0f) / 2;
            }
         } else {
            temp = pow((1.0f / 255.0f) * i, gamma);
         }
      }
      tbl1[i] = (int)(temp * BASE);
   }

   tbl1[i] = BASE;

   for (i = 0; i <= sizeof tbl2 - 1; ++i) {
      tbl2[i] = rconv1(i * (BASE / (sizeof tbl2 - 1)));
   }

   const int* table = pSettings->GetTuneTable();
   const int* tableR = pSettings->GetTuneTableR();
   const int* tableG = pSettings->GetTuneTableG();
   const int* tableB = pSettings->GetTuneTableB();
   const int* shadow = pSettings->GetShadowParams();
   const int paramalpha = Max(shadow[2], 1);

   for (i = 0; i < 256; ++i) {
      tunetbl[i] = alphatbl[Bound(table[i], 0, 255)];
      tunetblR[i] = alphatbl[Bound(tableR[i], 0, 255)];
      tunetblG[i] = alphatbl[Bound(tableG[i], 0, 255)];
      tunetblB[i] = alphatbl[Bound(tableB[i], 0, 255)];
      tunetblS[i] = alphatbl[Bound(table[i] / paramalpha, 0, 255)];
      tunetblRS[i] = alphatbl[Bound(tableR[i] / paramalpha, 0, 255)];
      tunetblGS[i] = alphatbl[Bound(tableG[i] / paramalpha, 0, 255)];
      tunetblBS[i] = alphatbl[Bound(tableB[i] / paramalpha, 0, 255)];
   }
}

BYTE CAlphaBlend::rconv1(int n)
{
   int pos = 0x80;
   int i = pos >> 1;
   while (i > 0) {
      if (n >= tbl1[pos]) {
         pos += i;
      } else {
         pos -= i;
      }
      i >>= 1;
   }
   if (n >= tbl1[pos]) {
      ++pos;
   }
   return (BYTE)(pos - 1);
}

class CAlphaBlendColorOne
{
private:
   BYTE fg;
   int temp_fg;
   const int *tunetbl;
   BYTE bg0;
   int alpha0;
   BYTE c0;
public:
   CAlphaBlendColorOne()
      : fg(0), temp_fg(0), tunetbl(NULL), bg0(0), alpha0(0), c0(0) {}
   void init(BYTE f, const int *tbl);
   ~CAlphaBlendColorOne() {};
   BYTE doAB(BYTE bg, int alpha);
};

FORCEINLINE void CAlphaBlendColorOne::init(BYTE f, const int *tbl)
{
   fg = f;
   temp_fg = s_AlphaBlendTable.conv1(fg);
   tunetbl = tbl;
}

FORCEINLINE BYTE CAlphaBlendColorOne::doAB(BYTE bg, int alpha)
{
   if (bg0 == bg && alpha0 == alpha) return c0;
   int temp_alpha = tunetbl[alpha];
   if (fg == bg || temp_alpha <= 0) return bg;
   if (temp_alpha >= s_AlphaBlendTable.BASE) return fg;
   int temp_bg = s_AlphaBlendTable.conv1(bg);
   int temp = temp_bg * (s_AlphaBlendTable.BASE - temp_alpha) +
         temp_fg * temp_alpha;
   bg0 = bg;
   alpha0 = alpha;
   return c0 = s_AlphaBlendTable.conv2(temp);
}

class CAlphaBlendColor
{
private:
   CAlphaBlendColorOne r;
   CAlphaBlendColorOne g;
   CAlphaBlendColorOne b;
public:
   CAlphaBlendColor(COLORREF newColor, int paramalpha, bool lcd, bool gbr = false);
   ~CAlphaBlendColor() { }
   BYTE doABsub(BYTE fg, int temp_fg, BYTE bg, int temp_alpha) const;
   COLORREF doAB(COLORREF baseColor, int alphaR, int alphaG, int alphaB);
   COLORREF doAB(COLORREF baseColor, int alpha) {
      return doAB(baseColor, alpha, alpha, alpha);
   }
private:
   CAlphaBlendColor() { }
};

FORCEINLINE CAlphaBlendColor::CAlphaBlendColor(COLORREF newColor, int paramalpha, bool lcd, bool gbr)
{
   const int *tblR;
   const int *tblG;
   const int *tblB;
   s_AlphaBlendTable.gettunetbl(paramalpha, lcd, tblR, tblG, tblB);
   if (!gbr) {
      r.init(GetRValue(newColor), tblR);
      b.init(GetBValue(newColor), tblB);
   } else {
      r.init(GetBValue(newColor), tblB);
      b.init(GetRValue(newColor), tblR);
   }
   g.init(GetGValue(newColor), tblG);
}

FORCEINLINE COLORREF CAlphaBlendColor::doAB(COLORREF baseColor, int alphaR, int alphaG, int alphaB)
{
   return RGB( r.doAB(GetRValue(baseColor), alphaR),
            g.doAB(GetGValue(baseColor), alphaG),
            b.doAB(GetBValue(baseColor), alphaB));
}

FORCEINLINE BYTE CAlphaBlend::doAB(BYTE fg, BYTE bg, int alpha)
{
   if (fg == bg || alpha <= 0) return bg;
   if (alpha >= 255) return fg;
   int temp_alpha = convalpha(alpha);
   int temp_bg = conv1(bg);
   int temp_fg = conv1(fg);
   int temp = temp_bg * (BASE - temp_alpha) +
         temp_fg * temp_alpha;
   return conv2(temp);
}

FORCEINLINE BYTE DoAlphaBlend(BYTE fg, BYTE bg, int alpha)
{
   return s_AlphaBlendTable.doAB(fg, bg, alpha);
}

// LCD(液晶)用のアルファブレンド(サブピクセルレンダリング)
static FORCEINLINE
COLORREF AlphaBlendColorLCD(
      COLORREF baseColor,
      COLORREF newColor,
      int alphaR, int alphaG, int alphaB,
      const int* tableR, const int* tableG, const int* tableB,
      const FreeTypeDrawInfo& ftdi)
{
   const BYTE rs = GetRValue(baseColor);
   const BYTE gs = GetGValue(baseColor);
   const BYTE bs = GetBValue(baseColor);
   BYTE rd = GetRValue(newColor);
   BYTE gd = GetGValue(newColor);
   BYTE bd = GetBValue(newColor);
   // アルファ値を補正
   alphaR = tableR[alphaR] / ftdi.params.alpha;
   alphaG = tableG[alphaG] / ftdi.params.alpha;
   alphaB = tableB[alphaB] / ftdi.params.alpha;
// rd = (((rd - rs) * alphaR) / 255) + rs;
// gd = (((gd - gs) * alphaG) / 255) + gs;
// bd = (((bd - bs) * alphaB) / 255) + bs;
   rd = DoAlphaBlend(rd, rs, alphaR);
   gd = DoAlphaBlend(gd, gs, alphaG);
   bd = DoAlphaBlend(bd, bs, alphaB);
   return RGB(rd, gd, bd);
}

// アルファブレンド(256階調)
static FORCEINLINE
COLORREF AlphaBlendColor(
      COLORREF baseColor,
      COLORREF newColor,
      int alpha, const int* table,
      const FreeTypeDrawInfo& ftdi)
{
   const BYTE rs = GetRValue(baseColor);
   const BYTE gs = GetGValue(baseColor);
   const BYTE bs = GetBValue(baseColor);
   BYTE rd = GetRValue(newColor);
   BYTE gd = GetGValue(newColor);
   BYTE bd = GetBValue(newColor);
   // アルファ値を補正
   alpha = table[alpha] / ftdi.params.alpha;
// rd = (rs * (255 - alpha) + rd * alpha) / 255;
// gd = (gs * (255 - alpha) + gd * alpha) / 255;
// bd = (bs * (255 - alpha) + bd * alpha) / 255;

// rd = (((rd - rs) * alpha) / 255) + rs;
// gd = (((gd - gs) * alpha) / 255) + gs;
// bd = (((bd - bs) * alpha) / 255) + bs;
   rd = DoAlphaBlend(rd, rs, alpha);
   gd = DoAlphaBlend(gd, gs, alpha);
   bd = DoAlphaBlend(bd, bs, alpha);
   return RGB(rd, gd, bd);
}

// 2階調
static void FreeTypeDrawBitmapPixelModeMono(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      int x, int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_MONO){
      return;
   }

   const COLORREF color = RGB2DIB(ftdi.Color());

   const SIZE cachebufsize = cache.Size();
   DWORD * const cachebufp = (DWORD *)cache.GetPixels();
   DWORD * cachebufrowp;

   int left, top, width, height;
   if (x < 0) {
      left = -x;
      x = 0;
   } else {
      left = 0;
   }
   width = Min(bitmap->width, (int)(cachebufsize.cx - x));
   top = 0;
   height = bitmap->rows;

   for(j = top, dy = y; j < height; ++j, ++dy){
      if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;
      p = bitmap->pitch < 0 ?
         &bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :  // up-flow
         &bitmap->buffer[bitmap->pitch * j]; // down-flow
      cachebufrowp = &cachebufp[dy * cachebufsize.cx];
      for(i = left, dx = x; i < width; ++i, ++dx){
         if((p[i / 8] & (1 << (7 - (i % 8)))) != 0){
            cachebufrowp[dx] = color;
         }
      }
   }
}

// LCD(液晶)用描画(サブピクセルレンダリング)
// RGB順(のはず)
static void FreeTypeDrawBitmapPixelModeLCD(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      int x, int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   COLORREF c;
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_LCD){
      return;
   }

   const COLORREF color = ftdi.Color();
   const int AAMode  = ftdi.pfs->GetAntiAliasMode();

   const SIZE cachebufsize = cache.Size();
   DWORD * const cachebufp = (DWORD *)cache.GetPixels();
   DWORD * cachebufrowp;

   // LCDは3サブピクセル分ある
   int left, top, width, height;
   if (x < 0) {
      left = -x * 3;
      x = 0;
   } else {
      left = 0;
   }
   width = Min(bitmap->width, (int)(cachebufsize.cx - x) * 3);
   top = 0;
   height = bitmap->rows;

   CAlphaBlendColor ab(color, ftdi.params.alpha, true, true);

   COLORREF backColor;
   int alphaR, alphaG, alphaB;

   for(j = 0, dy = y; j < height; ++j, ++dy){
      if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;

      p = bitmap->pitch < 0 ?
         &bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :  // up-flow
         &bitmap->buffer[bitmap->pitch * j]; // down-flow

      cachebufrowp = &cachebufp[dy * cachebufsize.cx];

      for(i = left, dx = x; i < width; i += 3, ++dx){
         if(AAMode == 2 || AAMode == 4){
            // これはRGBの順にサブピクセルがあるディスプレイ用
            alphaR = p[i + 0];
            alphaG = p[i + 1];
            alphaB = p[i + 2];
         }else{
            // BGR
            alphaR = p[i + 2];
            alphaG = p[i + 1];
            alphaB = p[i + 0];
         }
         backColor = cachebufrowp[dx];
         c = ab.doAB(backColor, alphaB, alphaG, alphaR);
         cachebufrowp[dx] = c;
      }
   }
}

// グリフビットマップのレンダリング
static void FreeTypeDrawBitmap(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      int x, int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   COLORREF c;
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_GRAY){
      // この関数自体はFT_PIXEL_MODE_GRAYにのみ対応し他に委譲する
      switch(bitmap->pixel_mode){
      case FT_PIXEL_MODE_MONO:
         FreeTypeDrawBitmapPixelModeMono(
            cache, bitmap, x, y, ftdi);
         break;
      case FT_PIXEL_MODE_LCD:
         FreeTypeDrawBitmapPixelModeLCD(
            cache, bitmap, x, y, ftdi);
         break;
      default:
         return;     // 未対応
      }
      return;
   }

   const COLORREF color = ftdi.Color();
   const SIZE cachebufsize = cache.Size();
   DWORD * const cachebufp = (DWORD *)cache.GetPixels();
   DWORD * cachebufrowp;

   int left, top, width, height;
   if (x < 0) {
      left = -x;
      x = 0;
   } else {
      left = 0;
   }
   width = Min(bitmap->width, (int)(cachebufsize.cx - x));
   top = 0;
   height = bitmap->rows;

   CAlphaBlendColor ab(color, ftdi.params.alpha, false, true);

   COLORREF backColor;
   int alpha;

   for(j = top, dy = y; j < height; ++j, ++dy){
      if ((unsigned int)dy >= (unsigned int)cachebufsize.cy) continue;
      p = bitmap->pitch < 0 ?
         &bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :  // up-flow
         &bitmap->buffer[bitmap->pitch * j]; // down-flow
      cachebufrowp = &cachebufp[dy * cachebufsize.cx];
      for(i = left, dx = x; i < width; ++i, ++dx){
         alpha = p[i];
         backColor = cachebufrowp[dx];
         c = ab.doAB(backColor, alpha);
         cachebufrowp[dx] = c;
      }
   }
}

// 縦書き用のレンダリング(コピペ手抜き)
// 2階調
static void FreeTypeDrawBitmapPixelModeMonoV(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      const int x, const int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_MONO){
      return;
   }

   const COLORREF color = ftdi.Color();

   const int width = bitmap->width;
   const int height = bitmap->rows;

   for(j = 0, dy = x; j < height; ++j, ++dy){
      p = bitmap->pitch < 0 ?
         &bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :  // up-flow
         &bitmap->buffer[bitmap->pitch * j]; // down-flow
      for(i = 0, dx = y+width; i < width; ++i, --dx){
         if((p[i / 8] & (1 << (7 - (i % 8)))) != 0){
            if (cache.GetPixel(dx, dy) != CLR_INVALID) { // dx dy エラーチェック
               cache.SetCurrentPixel(color);
            }
         }
      }
   }
}

// LCD(液晶)用描画(サブピクセルレンダリング)
// RGB順(のはず)
static void FreeTypeDrawBitmapPixelModeLCDV(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      const int x, const int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   COLORREF c;
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_LCD_V){
      return;
   }

   const COLORREF color = ftdi.Color();
   const int AAMode  = ftdi.pfs->GetAntiAliasMode();

   // LCDは3サブピクセル分ある
   const int width = bitmap->width;
   const int height = bitmap->rows;
   const int pitch = bitmap->pitch;
   const int pitchabs = pitch < 0 ? -pitch : pitch;

   CAlphaBlendColor ab(color, ftdi.params.alpha, true);

   for(j = 0, dy = x; j < height; j += 3, ++dy){
      p = pitch < 0 ?
         &bitmap->buffer[(pitchabs * bitmap->rows) + pitchabs * j] : // up-flow
         &bitmap->buffer[pitchabs * j];   // down-flow

      int alphaR, alphaG, alphaB;
      for(i = 0, dx = y+width; i < width; ++i, --dx){
         const COLORREF backColor = cache.GetPixel(dy, dx);

         if (backColor == color || backColor == CLR_INVALID) continue;
         if(AAMode == 2 || AAMode == 4){
            // これはRGBの順にサブピクセルがあるディスプレイ用
            alphaR = p[i + 0];
            alphaG = p[i + pitch];
            alphaB = p[i + pitch * 2];
         }else{
            // BGR
            alphaR = p[i + pitch * 2];
            alphaG = p[i + pitch];
            alphaB = p[i + 0];
         }
         c = ab.doAB(backColor, alphaR, alphaG, alphaB);
         cache.SetCurrentPixel(c);
      }

      if (i >= width)
         continue;
   }
}

static void FreeTypeDrawBitmapV(
      CBitmapCache& cache,
      const FT_Bitmap *bitmap,
      const int x, const int y,
      const FreeTypeDrawInfo& ftdi)
{
   int i, j;
   int dx, dy; // display
   int width, height;
   COLORREF c;
   FT_Bytes p;

   if(bitmap->pixel_mode != FT_PIXEL_MODE_GRAY){
      // この関数自体はFT_PIXEL_MODE_GRAYにのみ対応し他に委譲する
      switch(bitmap->pixel_mode){
      case FT_PIXEL_MODE_MONO:
         FreeTypeDrawBitmapPixelModeMonoV(
            cache, bitmap, x, y, ftdi);
         break;
      case FT_PIXEL_MODE_LCD_V:
         FreeTypeDrawBitmapPixelModeLCDV(
            cache, bitmap, x, y, ftdi);
         break;
      default:
         return;     // 未対応
      }
      return;
   }

   const COLORREF color = ftdi.Color();
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   const int* table = pSettings->GetTuneTable();
   width = bitmap->width;
   height = bitmap->rows;

   CAlphaBlendColor ab(color, ftdi.params.alpha, false);

   for(j = 0, dy = x; j < height; ++j, ++dy){
      p = bitmap->pitch < 0 ?
         &bitmap->buffer[(-bitmap->pitch * bitmap->rows) - bitmap->pitch * j] :  // up-flow
         &bitmap->buffer[bitmap->pitch * j]; // down-flow
      for(i = 0, dx = y+width; i < width; ++i, --dx){
         const COLORREF backColor = cache.GetPixel(dy, dx);
         if (backColor == color || backColor == CLR_INVALID) continue;
         c = ab.doAB(backColor, p[i]);
         cache.SetPixelV(dy, dx, c);
      }
   }
}

class CGGOGlyphLoader
{
private:
   FT_Library m_lib;
   const FT_Glyph_Class *m_clazz;
   BYTE bgtbl[0x41];
   static int CALLBACK EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam);
public:
   CGGOGlyphLoader() : m_lib(NULL), m_clazz(NULL) {}
   ~CGGOGlyphLoader() {}
   bool init(FT_Library freetype_library);
   FT_Library getlib() { return m_lib; }
   const FT_Glyph_Class * getclazz() { return m_clazz; }
   BYTE convbgpixel(BYTE val) { return bgtbl[val]; }
};
static CGGOGlyphLoader s_GGOGlyphLoader;

int CALLBACK CGGOGlyphLoader::EnumFontFamProc(const LOGFONT* lplf, const TEXTMETRIC* lptm, DWORD FontType, LPARAM lParam)
{
   CGGOGlyphLoader* pThis = reinterpret_cast<CGGOGlyphLoader*>(lParam);
   if (FontType != TRUETYPE_FONTTYPE || lplf->lfCharSet == SYMBOL_CHARSET) {
      return TRUE;
   }

   FreeTypeSysFontData* pFont = FreeTypeSysFontData::CreateInstance(lplf->lfFaceName, 0, false);
   if (!pFont) {
      return TRUE;
   }

   const FT_Glyph_Class *clazz = NULL;
   FT_Face face = pFont->GetFace();
   FT_Error err = FT_Set_Pixel_Sizes(face, 0, 9);
   if (!err) {
      err = FT_Load_Char(face, lptm->tmDefaultChar, FT_LOAD_NO_BITMAP);
      if (!err) {
         FT_Glyph glyph;
         err = FT_Get_Glyph(face->glyph, &glyph);
         if (!err) {
            if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
               clazz = glyph->clazz;
            }
            FT_Done_Glyph(glyph);
         }
      }
   }

   FT_Done_Face(face);

   if (clazz) {
      pThis->m_clazz = clazz;
      //列挙中止
      return FALSE;
   }
   return TRUE;
}

bool
CGGOGlyphLoader::init(FT_Library freetype_library)
{
   if (m_lib) {
      return true;
   }
   
   if (!freetype_library) {
      return false;
   }

   for (BYTE val = 0; val <= 0x40; ++val) {
      BYTE t = (BYTE)(((DWORD)val * 256) / 65);
      bgtbl[val] = t + (t >> 6);
   }

   m_lib = freetype_library;
   m_clazz = NULL;

   //前の方法だと、arial.ttfが無いとまずそうなので
   //適当に使えるアウトラインフォントを探す
   HDC hdc = CreateCompatibleDC(NULL);
   EnumFontFamilies(hdc, NULL, EnumFontFamProc, reinterpret_cast<LPARAM>(this));
   DeleteDC(hdc);

   if (m_clazz != NULL) {
      return true;
   }
   m_lib = NULL;
   return false;
}

class CGGOOutlineGlyph
{
private:
   FT_OutlineGlyph m_ptr;
   static FT_F26Dot6 toF26Dot6(const FIXED& fx) {
      return *(LONG *)(&fx) >> 10;
   }
   static FT_Fixed toFixed(const short n) {
      return (FT_Fixed)n << 16;
   }
   static char getTag(char tag, const FT_Vector& point) {
      if ((point.x & 0x0f) != 0) {
         tag |= FT_CURVE_TAG_TOUCH_X;
      }
      if ((point.y & 0x0f) != 0) {
         tag |= FT_CURVE_TAG_TOUCH_Y;
      }
      return tag;
   }
public:
   CGGOOutlineGlyph() : m_ptr(NULL) { _ASSERTE(s_GGOGlyphLoader.getlib()); }
   ~CGGOOutlineGlyph() { done(); };
   bool init(DWORD bufsize, PVOID bufp, const GLYPHMETRICS& gm);
   void done();
   operator FT_Glyph () { return (FT_Glyph)m_ptr; }
};

void
CGGOOutlineGlyph::done()
{
   if (m_ptr) {
      free(m_ptr->outline.points);
      free(m_ptr->outline.tags);
      free(m_ptr->outline.contours);
   }
   free(m_ptr);
   m_ptr = NULL;
}

bool
CGGOOutlineGlyph::init(DWORD bufsize, PVOID bufp, const GLYPHMETRICS& gm)
{
   done();
   m_ptr = (FT_OutlineGlyph)calloc(1, sizeof *m_ptr);
   if (!m_ptr) {
      return false;
   }

   FT_GlyphRec& root = m_ptr->root;
   FT_Outline& outline = m_ptr->outline;

   root.library = s_GGOGlyphLoader.getlib();
   root.clazz = s_GGOGlyphLoader.getclazz();
   root.format = FT_GLYPH_FORMAT_OUTLINE;
   root.advance.x = toFixed(gm.gmCellIncX);
   root.advance.y = toFixed(gm.gmCellIncY);

   outline.n_contours = 0;
   outline.n_points = 0;
   outline.flags = 0; //FT_OUTLINE_HIGH_PRECISION;

   LPTTPOLYGONHEADER ttphp = (LPTTPOLYGONHEADER)bufp;
   LPTTPOLYGONHEADER ttphpend = (LPTTPOLYGONHEADER)((PBYTE)ttphp + bufsize);

   while (ttphp < ttphpend) {
      LPTTPOLYCURVE ttpcp = (LPTTPOLYCURVE)(ttphp + 1);
      LPTTPOLYCURVE ttpcpend = (LPTTPOLYCURVE)((PBYTE)ttphp + ttphp->cb);
      if ((PVOID)ttpcpend > (PVOID)ttphpend) {
         break;
      }
      ++outline.n_points;
      ++outline.n_contours;
      while (ttpcp < ttpcpend) {
         LPPOINTFX pfxp = &ttpcp->apfx[0];
         outline.n_points += ttpcp->cpfx;
         ttpcp = (LPTTPOLYCURVE)(pfxp + ttpcp->cpfx);
      }
      ttphp = (LPTTPOLYGONHEADER)ttpcp;
   }

   if (ttphp != ttphpend) {
      return false;
   }
   outline.points = (FT_Vector *)calloc(outline.n_points, sizeof *outline.points);
   outline.tags = (char *)calloc(outline.n_points, sizeof *outline.tags);
   outline.contours = (short *)calloc(outline.n_contours, sizeof *outline.contours);
   if (!outline.points || !outline.tags || !outline.contours) {
      done();
      return false;
   }

   short *cp = outline.contours;
   short ppos = 0;

   ttphp = (LPTTPOLYGONHEADER)bufp;
   while (ttphp < ttphpend) {
      LPTTPOLYCURVE ttpcp = (LPTTPOLYCURVE)(ttphp + 1);
      LPTTPOLYCURVE ttpcpend = (LPTTPOLYCURVE)((PBYTE)ttphp + ttphp->cb);

      LPPOINTFX pfxp0 = &ttpcp->apfx[0];
      while (ttpcp < ttpcpend) {
         LPPOINTFX pfxp = &ttpcp->apfx[0];
         pfxp0 = pfxp + (ttpcp->cpfx - 1);
         ttpcp = (LPTTPOLYCURVE)(pfxp + ttpcp->cpfx);
      }
      ttpcp = (LPTTPOLYCURVE)(ttphp + 1);

      if (pfxp0->x.value != ttphp->pfxStart.x.value || pfxp0->x.fract != ttphp->pfxStart.x.fract ||
         pfxp0->y.value != ttphp->pfxStart.y.value || pfxp0->y.fract != ttphp->pfxStart.y.fract) {
         outline.points[ppos].x = toF26Dot6(ttphp->pfxStart.x);
         outline.points[ppos].y = toF26Dot6(ttphp->pfxStart.y);
         outline.tags[ppos] = getTag(FT_CURVE_TAG_ON, outline.points[ppos]);
         ++ppos;
      }
      while (ttpcp < ttpcpend) {
         char tag;
         switch (ttpcp->wType) {
         case TT_PRIM_LINE:
            tag = FT_CURVE_TAG_ON;
            break;
         case TT_PRIM_QSPLINE:
            tag = FT_CURVE_TAG_CONIC;
            break;
         case TT_PRIM_CSPLINE:
            tag = FT_CURVE_TAG_CONIC;
            break;
         default:
            tag = 0;
         }

         LPPOINTFX pfxp = &ttpcp->apfx[0];
         for (WORD cnt = 0; cnt < ttpcp->cpfx; ++cnt) {
            outline.points[ppos].x = toF26Dot6(pfxp->x);
            outline.points[ppos].y = toF26Dot6(pfxp->y);
            outline.tags[ppos] = tag;
            ++ppos;
            ++pfxp;
         }
         outline.tags[ppos - 1] = getTag(FT_CURVE_TAG_ON, outline.points[ppos - 1]);
         ttpcp = (LPTTPOLYCURVE)pfxp;
      }
      *cp++ = ppos - 1;
      ttphp = (LPTTPOLYGONHEADER)ttpcp;
   }
   outline.n_points = ppos;
   return true;
}

template<typename T>
class CTempMem
{
private:
   char m_localbuf[0x0f80];
   DWORD m_size;
   T m_ptr;
public:
   CTempMem() : m_size(sizeof m_localbuf), m_ptr((T)m_localbuf) {
   }
   ~CTempMem() {
      done();
   }
   T init(DWORD size) {
      done();
      if (m_size > size) {
         m_size = size;
         m_ptr = (T)malloc(m_size);
      }
      return m_ptr;
   }
   void done() {
      if (m_ptr != (T)m_localbuf) {
         free(m_ptr);
      }
      m_size = sizeof m_localbuf;
      m_ptr = (T)m_localbuf;
   }
   operator T () { return m_ptr; }
   bool operator ! () { return !m_ptr; }
   DWORD getsize() { return m_size; }
};

BOOL FreeTypePrepare(FreeTypeDrawInfo& FTInfo)
{
   FT_Face& freetype_face        = FTInfo.freetype_face;
   FT_Int& cmap_index            = FTInfo.cmap_index;
   FT_Render_Mode& render_mode      = FTInfo.render_mode;
   FTC_ImageTypeRec& font_type      = FTInfo.font_type;
   FreeTypeFontInfo*& pfi        = FTInfo.pfi;
   const CFontSettings*& pfs     = FTInfo.pfs;
   FreeTypeFontCache*& pftCache  = FTInfo.pftCache;

   FTC_FaceID face_id = NULL;
   int height = 0;

   const LOGFONTW& lf = FTInfo.LogFont();
   render_mode    = FT_RENDER_MODE_NORMAL;
   if (FTInfo.params.alpha < 1)
      FTInfo.params.alpha = 1;

   FTInfo.face_id_list_num = 0;
   //Assert(_tcsicmp(lf.lfFaceName, _T("@Arial Unicode MS")) != 0);
   pfi = NULL;
   bool f_addfont = false;
   do {
      CFontFaceNamesEnumerator fn(lf.lfFaceName);
      for ( ; !fn.atend(); fn.next()) {
         FreeTypeFontInfo* pfitemp = g_pFTEngine->FindFont(fn, lf.lfWeight, !!lf.lfItalic);
         if (pfitemp) {
            if (!pfi) pfi = pfitemp;
            FTInfo.face_id_list[FTInfo.face_id_list_num++] = (FTC_FaceID)pfitemp->GetId();
         }
      }
      if (!f_addfont) {
         AddFontToFT(lf.lfFaceName, lf.lfWeight, !!lf.lfItalic);
         f_addfont = true;
         continue;
      }
   } while (0);

   if (!pfi) {
      return FALSE;
   }

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   pfs = &pfi->GetFontSettings();
   // グリフ切り替え情報取得のため常に必要
   face_id = (FTC_FaceID)pfi->GetId();
   if (FTC_Manager_LookupFace(cache_man, face_id, &freetype_face))
      return FALSE;
   cmap_index = FT_Get_Charmap_Index(freetype_face->charmap);
   switch (pSettings->FontLoader()) {
   case SETTING_FONTLOADER_FREETYPE:
      {
         FTC_ScalerRec scaler = { 0 };
         scaler.face_id = face_id;
         scaler.width   = lf.lfWidth;
         if(lf.lfHeight > 0){
            // セル高さ
            scaler.height = MulDiv(lf.lfHeight, freetype_face->units_per_EM, freetype_face->height);
         }
         else{
            // 文字高さ
            scaler.height = -lf.lfHeight;
         }
         scaler.pixel = 1;
         scaler.x_res = 0;
         scaler.y_res = 0;

         FT_Size font_size;
         if(FTC_Manager_LookupSize(cache_man, &scaler, &font_size))
            return FALSE;
         height = scaler.height;
         break;
      }
   case SETTING_FONTLOADER_WIN32:
      {
         OUTLINETEXTMETRIC otm;
         if (GetOutlineTextMetrics(FTInfo.hdc, sizeof otm, &otm) != sizeof otm) {
            return FALSE;
         }
         height = lf.lfHeight;
      }
      break;
   default:
      return FALSE;
   }

   pftCache = pfi->GetCache(height, lf);
   if(!pftCache)
      return FALSE;

   /*FT_Size_RequestRec size_request;
   size_request.width = lf.lfWidth;
   size_request.horiResolution = 0;
   size_request.vertResolution = 0;
   if(lf.lfHeight > 0){
      // セル高さ
      size_request.type = FT_SIZE_REQUEST_TYPE_CELL;
      size_request.height = lf.lfHeight * 64;
   }
   else{
      // 文字高さ
      size_request.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
      size_request.height = (-lf.lfHeight) * 64;
   }
   if(FT_Request_Size(freetype_face, &size_request))
      goto Exit2;*/

   switch (pSettings->FontLoader()) {
   case SETTING_FONTLOADER_FREETYPE:
      // font_typeを設定
      font_type.face_id = face_id;
      font_type.width   = freetype_face->size->metrics.x_ppem;
      font_type.height  = freetype_face->size->metrics.y_ppem;

      /* ビットマップまでキャッシュする場合はFT_LOAD_RENDER | FT_LOAD_TARGET_*
       * とする。ただし途中でTARGETを変更した場合等はキャッシュが邪魔する。
       * そういう時はFT_LOAD_DEFAULTにしてFTC_ImageCache_Lookup後に
       * FT_Glyph_To_Bitmapしたほうが都合がいいと思う。
       */
      // Boldは太り具合というものがあるので本当はこれだけでは足りない気がする。
      /*if(IsFontBold(lf) && !(freetype_face->style_flags & FT_STYLE_FLAG_BOLD) ||
         lf.lfItalic && !(freetype_face->style_flags & FT_STYLE_FLAG_ITALIC)){
         // ボールド、イタリックは後でレンダリングする
         // 多少速度は劣化するだろうけど仕方ない。
         font_type.flags = FT_LOAD_NO_BITMAP;
      }
      else{
         font_type.flags = FT_LOAD_RENDER | FT_LOAD_NO_BITMAP;
      }*/
      break;
   case SETTING_FONTLOADER_WIN32:
      font_type.face_id = face_id;
      font_type.width   = -1;
      font_type.height  = -1;
      break;

   DEFAULT_UNREACHABLE;
   }
   font_type.flags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;

   // ヒンティング
   switch (pfs->GetHintingMode()) {
   case 0:
      // ignore.
      break;
   case 1:
      font_type.flags |= FT_LOAD_NO_HINTING;
      break;
   case 2:
      font_type.flags |= FT_LOAD_FORCE_AUTOHINT;
      break;
   }
   // アンチエイリアス
   if (FTInfo.IsMono()) {
      font_type.flags |= FT_LOAD_TARGET_MONO;
      render_mode = FT_RENDER_MODE_MONO;
   } else {
      switch (pfs->GetAntiAliasMode()) {
      case -1:
         font_type.flags |= FT_LOAD_TARGET_MONO;
         render_mode = FT_RENDER_MODE_MONO;
         break;
      case 0:
         font_type.flags |= FT_LOAD_TARGET_NORMAL;
         render_mode = FT_RENDER_MODE_NORMAL;
         break;
      case 1:
         font_type.flags |= FT_LOAD_TARGET_LIGHT;
         render_mode = FT_RENDER_MODE_LIGHT;
         break;
      case 2:
      case 3:
         font_type.flags |= FT_LOAD_TARGET_LCD;
         render_mode = FT_RENDER_MODE_LCD;
         break;
      case 4:
      case 5:
         font_type.flags |= FT_LOAD_TARGET_LIGHT;
         render_mode = FT_RENDER_MODE_LCD;
         break;
      }
   }

   FTInfo.useKerning = FALSE;
   if (pfs->GetKerning()) {
      switch (pSettings->FontLoader()) {
      case SETTING_FONTLOADER_FREETYPE:
         FTInfo.useKerning = !!FT_HAS_KERNING(freetype_face);
         break;
      case SETTING_FONTLOADER_WIN32:
         {
            DWORD rc = GetFontLanguageInfo(FTInfo.hdc);
            if (rc != GCP_ERROR) {
               FTInfo.useKerning = !!(rc & GCP_USEKERNING);
               FTInfo.ggokerning.init(FTInfo.hdc);
            }
         }
         break;

      DEFAULT_UNREACHABLE;
      }
   }
   return TRUE;
}

// 縦にするやつはtrue(ASCIIと半角カナはfalse)
inline bool IsVerticalChar(WCHAR wch){
   if(wch < 0x80)
      return false;
   if(0xFF61 <= wch && wch <= 0xFF9F)
      return false;
   // 本当はもっと真面目にやらないとまずいが。
   return true;
}

struct CGGOFont
{
   HDC m_hdc;
   HFONT m_hfont;
   HFONT m_hprevfont;
   CGGOFont(HDC hdc, const LOGFONT& olf) : m_hdc(hdc), m_hfont(NULL), m_hprevfont(NULL) {
      LOGFONT lf = olf;
      lf.lfWeight = FW_REGULAR;
      lf.lfItalic = FALSE;
      lf.lfStrikeOut = FALSE;
      m_hfont = CreateFontIndirect(&lf);
   }
   ~CGGOFont() {
      if (m_hprevfont) {
         SelectFont(m_hdc, m_hprevfont);
      }
      DeleteFont(m_hfont);
   }
   void change() {
      m_hprevfont = SelectFont(m_hdc, m_hfont);
   }
   void restore() {
      SelectFont(m_hdc, m_hprevfont);
      m_hprevfont = NULL;
   }
   operator HFONT () { return m_hfont; }
};

class ClpDx
{
private:
   const INT *p;
   const INT step;
public:
   ClpDx(const INT *lpDx, UINT etoOptions) : p(lpDx), step((etoOptions & ETO_PDY) ? 2 : 1) {
   }
   ~ClpDx() {
   }
   int get(int val) {
      int result;
      if (p) {
         result = *p;
         p += step;
      } else {
         result = val;
      }
      return result;
   }
};

BOOL ForEachGetGlyph(FreeTypeDrawInfo& FTInfo, LPCTSTR lpString, int cbString, BOOL (CALLBACK *pfnCallback)(FreeTypeDrawInfo&, const WCHAR wch, const FT_BitmapGlyph))
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   const FT_Face freetype_face = FTInfo.freetype_face;
   const FT_Int cmap_index = FTInfo.cmap_index;
   const FT_Bool useKerning = FTInfo.useKerning;
   FT_Render_Mode render_mode = FTInfo.render_mode;
   const int AAMode = FTInfo.pfs->GetAntiAliasMode();
   const LOGFONTW& lf = FTInfo.LogFont();
   FreeTypeFontCache* pftCache = FTInfo.pftCache;
   const bool bGlyphIndex = FTInfo.IsGlyphIndex();
   const bool bSizeOnly = FTInfo.IsSizeOnly();
   //const bool bOwnCache = !(FTInfo.font_type.flags & FT_LOAD_RENDER);
   const LPCTSTR lpStart = lpString;
   const LPCTSTR lpEnd = lpString + cbString;
   FT_UInt previous = 0;
   WCHAR previouswch = 0;
   const bool bVertical = lf.lfFaceName[0] == _T('@');
   const bool bLcdMode = render_mode == FT_RENDER_MODE_LCD;
   const bool bLightLcdMode = (AAMode == 4) || (AAMode == 5);
   ClpDx clpdx(FTInfo.lpDx, FTInfo.params.etoOptions);
   const bool bWidthGDI32 = pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32;
   const int ggoformatbase = (FTInfo.font_type.flags & FT_LOAD_NO_HINTING) ? GGO_UNHINTED | GGO_NATIVE : GGO_NATIVE;

   if (!s_GGOGlyphLoader.init(freetype_library)) {
      return FALSE;
   }

   for ( ; lpString < lpEnd; ++lpString){
      const WCHAR wch = *lpString;

      int gdi32x;
      if (!ORIG_GetCharWidth32W(FTInfo.hdc, wch, wch, &gdi32x)) return FALSE;

      FTInfo.font_type.face_id = FTInfo.face_id_list[0];

      FreeTypeCharData* chData = NULL;
      FT_BitmapGlyph glyph_bitmap = NULL;
      FT_Glyph copy_glyph = NULL;
      FT_UInt glyph_index = 0;

      //if (bSizeOnly || bOwnCache) {
      chData = bGlyphIndex
         ? pftCache->FindGlyphIndex(wch)
         : pftCache->FindChar(wch);
      //}

      if (chData) {
         if (bSizeOnly) {
            int cx = chData->GetWidth();
            FTInfo.x += (bWidthGDI32 ? gdi32x : cx) + FTInfo.params.charExtra;
            continue;
         }
         glyph_bitmap = (FT_BitmapGlyph)chData->GetGlyph(render_mode);
      }
      if (!glyph_bitmap) {
         FT_Glyph glyph = NULL;

         // 合成やBMP外の文字はUniscribeが処理するのでここで考慮する必要はない。
         bool f_glyph = false;
         GLYPHMETRICS gm;
         const MAT2 mat2 = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
         UINT ggoformat = ggoformatbase;
         CTempMem<PVOID> ggobuf;
         DWORD outlinesize = 0;
         
         switch (pSettings->FontLoader()) {
         case SETTING_FONTLOADER_FREETYPE:
            if (bGlyphIndex) {
               f_glyph = true;
               glyph_index = wch;
            } else if (wch) {
               for (int i = 0; i < FTInfo.face_id_list_num; ++i) {
                  glyph_index = FTC_CMapCache_Lookup(
                     cmap_cache,
                     FTInfo.face_id_list[i],
                     cmap_index,
                     wch);
                  if (glyph_index != 0) {
                     f_glyph = true;
                     FTInfo.font_type.face_id = FTInfo.face_id_list[i];
                     break;
                  }
               }
            }
            break;
         case SETTING_FONTLOADER_WIN32:
            if (bGlyphIndex) {
               f_glyph = true;
               glyph_index = wch;
               ggoformat |= GGO_GLYPH_INDEX;
            } else {
               // グリフインデックス取得
               WORD gi = 0;
               if (GetGlyphIndices(FTInfo.hdc, &wch, 1, &gi, GGI_MARK_NONEXISTING_GLYPHS) == 1) {
                  if (gi != 0xffff) {
                     glyph_index = gi;
                     f_glyph = true;
                  }
               }
            }
            if (lpString == lpStart && FTInfo.font_type.flags & FT_LOAD_FORCE_AUTOHINT) {
               // FORCE_AUTOHINT と関係ない文字化け対策
               GetGlyphOutline(FTInfo.hdc, 0, GGO_METRICS | GGO_GLYPH_INDEX | GGO_NATIVE | GGO_UNHINTED, &gm, 0, NULL, &mat2);
            }
            outlinesize = GetGlyphOutline(FTInfo.hdc, wch, ggoformat, &gm, ggobuf.getsize(), ggobuf, &mat2);
            if (outlinesize == GDI_ERROR || outlinesize == 0) {
               glyph_index = 0;
               f_glyph = false;
            } else {
               // グリフインデックスが取れないのにアウトラインが取れた
               f_glyph = true;
            }
            break;
         }

         if (!f_glyph) {
            int cx = gdi32x;
            if (bSizeOnly) {
               FTInfo.x += cx;
            } else {
               if (wch) {
                  ORIG_ExtTextOutW(FTInfo.hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO(), NULL, &wch, 1, NULL);
               }
               if (lpString < lpEnd - 1) {
                  FTInfo.x += clpdx.get(cx);
               } else {
                  ABC abc = {0, gdi32x, 0};
                  GetCharABCWidths(FTInfo.hdc, wch, wch, &abc);
                  FTInfo.x += Max(clpdx.get(cx), abc.abcA + (int)abc.abcB);
               }
            }
            FTInfo.x += FTInfo.params.charExtra;
            continue;
         }

         switch (pSettings->FontLoader()) {
         case SETTING_FONTLOADER_FREETYPE:
            // 縦書き
            if(bVertical){
               glyph_index = ft2vert_get_gid(
                  (struct ft2vert_st *)freetype_face->generic.data,
                  glyph_index);
            }

            // カーニング
            if(useKerning){
               if(previous != 0 && glyph_index){
                  FT_Vector delta;
                  FT_Get_Kerning(freetype_face,
                              previous, glyph_index,
                              ft_kerning_default, &delta);
                  FTInfo.x += FT_PosToInt(delta.x);
               }
               previous = glyph_index;
            }
            break;
         case SETTING_FONTLOADER_WIN32:
            if(useKerning && !bGlyphIndex){
               if (previouswch && wch) {
                  FTInfo.x += FTInfo.ggokerning.get(previouswch, wch);
               }
               previouswch = wch;
            }
            break;
         }

         // 縦横
         if(bVertical && IsVerticalChar(wch)){
            FTInfo.font_type.flags |= FT_LOAD_VERTICAL_LAYOUT;
            if(bLcdMode){
               if(!bLightLcdMode){
                  FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD;
                  FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD_V;
               }
               render_mode             = FT_RENDER_MODE_LCD_V;
            }
         }else{
            FTInfo.font_type.flags &=~FT_LOAD_VERTICAL_LAYOUT;
            if(bLcdMode){
               if(!bLightLcdMode){
                  FTInfo.font_type.flags &= ~FT_LOAD_TARGET_LCD_V;
                  FTInfo.font_type.flags |= FT_LOAD_TARGET_LCD;
               }
               render_mode             = FT_RENDER_MODE_LCD;
            }
         }

         CGGOOutlineGlyph ggoog;
         switch (pSettings->FontLoader()) {
         case SETTING_FONTLOADER_FREETYPE:
            if(FTC_ImageCache_Lookup(
                  image_cache,
                  &FTInfo.font_type,
                  glyph_index,
                  &glyph,
                  NULL)) {
               return FALSE;
            }
            break;
         case SETTING_FONTLOADER_WIN32:
            if (outlinesize > ggobuf.getsize()) {
               if (!ggobuf.init(outlinesize)) {
                  return FALSE;
               }
               //ggofont.change();
               outlinesize = GetGlyphOutline(FTInfo.hdc, wch, ggoformat, &gm, ggobuf.getsize(), ggobuf, &mat2);
               //ggofont.restore();
            }
            if (outlinesize > ggobuf.getsize()) {
               return FALSE;
            }
            if (!ggoog.init(outlinesize, ggobuf, gm)) {
               return FALSE;
            }
            glyph = ggoog;
            break;
         }
         /* font_type.flagsでFT_LOAD_RENDERを選択しないと
          * グリフイメージを取得してくるのでビットマップに変換する。
          */
         if(glyph->format == FT_GLYPH_FORMAT_BITMAP){
            glyph_bitmap = (FT_BitmapGlyph)glyph;
         }
         else{
            //FT_Glyph copy_glyph = NULL;
            if(FT_Glyph_Copy(glyph, &copy_glyph))
               return FALSE;

            // アウトラインを変形させる(上記サイトから)。
            int str_h;
            int str_v;
            bool fbold = false;
            str_h = str_v = FTInfo.pfi->CalcNormalWeight();
            if (IsFontBold(lf) &&
                pSettings->FontLoader() == SETTING_FONTLOADER_FREETYPE &&
                !(freetype_face->style_flags & FT_STYLE_FLAG_BOLD)) {
               fbold = true;
               str_h = FTInfo.pfi->GetFTWeight();
            }
            if((str_h || str_v) && New_FT_Outline_Embolden(
                  &((FT_OutlineGlyph)copy_glyph)->outline,
                  str_h, str_v))
               {
                  FT_Done_Glyph(copy_glyph);
                  return FALSE;     // 変換失敗
               }
            /*if (copy_glyph->advance.x) {
              FT_Matrix matrix;
              matrix.xx = (1 << 16) + (0x80000000 / (copy_glyph->advance.x >> 1));
              matrix.xy = 0;
              matrix.yx = 0;
              matrix.yy = 1 << 16;
              FT_Outline_Transform(
              &((FT_OutlineGlyph)copy_glyph)->outline,
              &matrix);
              }*/
            if (fbold) {
               ((FT_BitmapGlyph)copy_glyph)->root.advance.x += 0x10000;
            }
            if(lf.lfItalic &&
               pSettings->FontLoader() == SETTING_FONTLOADER_FREETYPE &&
               !(freetype_face->style_flags & FT_STYLE_FLAG_ITALIC)){
               // 斜体
               FT_Matrix matrix;
               // 変換
               FTInfo.pfi->CalcItalicSlant(matrix);
               FT_Outline_Transform(
                  &((FT_OutlineGlyph)copy_glyph)->outline,
                  &matrix);
            }

            //// 文字の幅をGDI32に合わせる
            //FT_Fixed& ftx = (bVertical && IsVerticalChar(wch)) ?
            //    copy_glyph->advance.y : copy_glyph->advance.x;
            //FT_Fixed targetx = gdi32x << 16;
            //if (ftx != targetx) {
            // if (ftx > targetx) {
            //    // 大きいなら文字縮小 (小さくても拡大はしない)
            //    FT_Matrix matrix = {FT_DivFix(targetx, ftx) , 0, 0, 1 << 16};
            //    FT_Outline_Transform(&((FT_OutlineGlyph)copy_glyph)->outline, &matrix);

            // }
            // ftx = targetx; // 大きくても小さくてもサイズあわせ
            //}

            /* FT_RENDER_MODE_NORMALで普通の(256階調)アンチエイリアス
             * FT_RENDER_MODE_LCDで液晶用アンチエイリアス(サブピクセルレンダリング)
             */

            if(FT_Glyph_To_Bitmap(&copy_glyph, render_mode, 0, 1)){
               FT_Done_Glyph(copy_glyph);
               return FALSE;
            }
            glyph_bitmap = (FT_BitmapGlyph)copy_glyph;
         }
      }

      int cx = (bVertical && IsVerticalChar(wch)) ?
            FT_FixedToInt(glyph_bitmap->root.advance.y) :
            FT_FixedToInt(glyph_bitmap->root.advance.x);
      pfnCallback(FTInfo, wch, glyph_bitmap);
      if (bSizeOnly) {
         FTInfo.x += bWidthGDI32 ? gdi32x : cx;
      } else {
         int dx = clpdx.get(bWidthGDI32 ? gdi32x : cx);
         if (lpString < lpEnd - 1) {
            FTInfo.x += dx;
         } else {
            int bx = glyph_bitmap->bitmap.width;
            if (render_mode == FT_RENDER_MODE_LCD) bx /= 3;
            bx += glyph_bitmap->left;
            FTInfo.x += Max(Max(dx, bx), cx);
         }
      }
      FTInfo.x += FTInfo.params.charExtra;

      //if (bSizeOnly || bOwnCache) {
         //キャッシュ化
      if (glyph_index) {
         if (bGlyphIndex) {
            pftCache->AddGlyphData(glyph_index, cx, glyph_bitmap, render_mode);
         } else {
            pftCache->AddCharData(wch, glyph_index, cx, glyph_bitmap, render_mode);
         }
      }
      //}

      if(copy_glyph){
         FT_Done_Glyph(copy_glyph);
      }
   }
   return TRUE;
}

BOOL GetLogFontFromDC(HDC hdc, LOGFONT& lf)
{
   LOGFONTW lfForce = { 0 };
   HFONT hf = GetCurrentFont(hdc);
   if (!GetObject(hf, sizeof(LOGFONTW), &lf))
      return FALSE;

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->CopyForceFont(lfForce, lf))
      lf = lfForce;

   if(pSettings->LoadOnDemand()) {
      AddFontToFT(lf.lfFaceName, lf.lfWeight, !!lf.lfItalic);
   }
   return TRUE;
}

BOOL CALLBACK TextOutCallback(FreeTypeDrawInfo& FTInfo, const WCHAR wch, const FT_BitmapGlyph glyph_bitmap)
{
   const bool bVertical = FTInfo.LogFont().lfFaceName[0] == _T('@');

   if (!glyph_bitmap->bitmap.buffer) {
      if (FTInfo.params.alpha == 1) {
         ORIG_ExtTextOutW(FTInfo.hdc, FTInfo.x, FTInfo.yTop, FTInfo.GetETO(), NULL, &wch, 1, NULL);
      }
   } else {
      const CGdippSettings* pSettings = CGdippSettings::GetInstance();
      if (bVertical && IsVerticalChar(wch) &&
         pSettings->FontLoader() == SETTING_FONTLOADER_FREETYPE) {
         FreeTypeDrawBitmapV(*FTInfo.pCache,
            &glyph_bitmap->bitmap,
            FTInfo.x + FTInfo.yBase - glyph_bitmap->top,
            FTInfo.yTop + FT_PosToInt(FTInfo.freetype_face->size->metrics.height) -
               (glyph_bitmap->left+glyph_bitmap->bitmap.width) - 1,
            FTInfo);
      }else{
         FreeTypeDrawBitmap(*FTInfo.pCache,
            &glyph_bitmap->bitmap,
            FTInfo.x + glyph_bitmap->left,
            FTInfo.yBase - glyph_bitmap->top,
            FTInfo);
      }
   }

   //if (FTInfo.lpDx) {
   // FTInfo.x += *FTInfo.lpDx++;
   // return FALSE;
   //}
   return TRUE;
}

BOOL CALLBACK GetTextExtentCallback(FreeTypeDrawInfo& /*FTInfo*/, const WCHAR /*wch*/, const FT_BitmapGlyph /*glyph_bitmap*/)
{
   return TRUE;
}

// FreeTypeを用いたTextOut APIもどき
BOOL FreeTypeTextOut(
   const HDC hdc,     // デバイスコンテキストのハンドル
   CBitmapCache& cache,
   const int nXStart, // 開始位置（基準点）の x 座標
   const int nYStart, // 開始位置（基準点）の y 座標
   LPCWSTR lpString,  // 文字列
   int cbString,      // 文字数
   const int* lpDx,   // 描画位置
   const FREETYPE_PARAMS* params,
   int& width
   )
{
   CCriticalSectionLock __lock;

   // Unicodeで渡すようにする(FT_Select_Charmapで変更可能)
   if(cbString <= 0 || lpString == NULL)
      return FALSE;

   LOGFONTW* lplf = NULL;
   LOGFONTW lfForce = { 0 };
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->CopyForceFont(lfForce, *params->lplf))
      lplf = &lfForce;

   FreeTypeDrawInfo FTInfo(*params, hdc, lplf, &cache, lpDx);
   if(!FreeTypePrepare(FTInfo))
      return FALSE;

   FT_Face freetype_face = FTInfo.freetype_face;
   const LOGFONT& lf = FTInfo.LogFont();

   FTInfo.x     = nXStart;
   FTInfo.yTop  = nYStart;

   const TEXTMETRIC& tm = FTInfo.pftCache->GetTextMetric(hdc);
   FTInfo.yBase = nYStart + tm.tmAscent;
   if(!ForEachGetGlyph(FTInfo, lpString, cbString, TextOutCallback))
      return FALSE;
   width = FTInfo.x;

   const int x = FTInfo.x;
   const int y = FTInfo.yBase;

   // 下線を(あれば)引く
   if(lf.lfUnderline || lf.lfStrikeOut) {
      OUTLINETEXTMETRIC otm = { sizeof(OUTLINETEXTMETRIC) };
      GetOutlineTextMetrics(hdc, otm.otmSize, &otm);
      if(lf.lfUnderline){
         int yPos = 0; //下線の位置
         int height = 0;
         int thickness = 0; // 適当な太さ
         switch (pSettings->FontLoader()) {
         case SETTING_FONTLOADER_FREETYPE:
            yPos = y + 1;
            height = FT_PosToInt(freetype_face->size->metrics.height);
            thickness =
               MulDiv(freetype_face->underline_thickness,
                  freetype_face->size->metrics.y_ppem,
                  freetype_face->units_per_EM);
            break;
         case SETTING_FONTLOADER_WIN32:
            yPos = y - otm.otmsUnderscorePosition;
            height = otm.otmTextMetrics.tmHeight;
            thickness = otm.otmsUnderscoreSize;
            break;
         }
         if (yPos >= height) {
            yPos = height - 1;
         }
         cache.DrawHorizontalLine(nXStart, yPos, x, FTInfo.Color(), thickness);
      }

      // 取消線を(あれば)引く
      if(lf.lfStrikeOut){
         // 取消線の位置(FreeTypeで取得できそうにないのでWin32API利用)
         int yPos = y - otm.otmsStrikeoutPosition; //取消線の位置
         int thickness = 0; // 適当な太さ
         switch (pSettings->FontLoader()) {
         case SETTING_FONTLOADER_FREETYPE:
            thickness =
               MulDiv(freetype_face->underline_thickness,
                  freetype_face->size->metrics.y_ppem,
                  freetype_face->units_per_EM);
            break;
         case SETTING_FONTLOADER_WIN32:
            thickness = otm.otmsStrikeoutSize;
            break;
         }
         cache.DrawHorizontalLine(nXStart, yPos, x, FTInfo.Color(), thickness);
      }
   }

//PatBlt(hdc, 0, y, 9999, y + 1, DSTINVERT);
//PatBlt(hdc, 0, 0, 9999, 0 + 1, DSTINVERT);
   return TRUE;
}

BOOL FreeTypeGetTextExtentPoint(
   const HDC hdc,
   LPCWSTR lpString,
   int cbString,
   LPSIZE lpSize,
   const FREETYPE_PARAMS* params
   )
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32) {
      return ORIG_GetTextExtentPoint32W(hdc, lpString, cbString, lpSize);
   }

   CCriticalSectionLock __lock;

   //とりあえず0埋め
   if (lpSize) {
      lpSize->cx = lpSize->cy = 0;
   }

   // Unicodeで渡すようにする(FT_Select_Charmapで変更可能)
   if(cbString <= 0 || lpString == NULL || lpSize == NULL) {
      return FALSE;
   }

   Assert(params != NULL);
   LOGFONT lf;
   if (!GetLogFontFromDC(hdc, lf))
      return FALSE;

   FreeTypeDrawInfo FTInfo(*params, hdc, &lf);
   if(!FreeTypePrepare(FTInfo))
      return FALSE;

   FTInfo.x = 0;
   if(!ForEachGetGlyph(FTInfo, lpString, cbString, GetTextExtentCallback)) {
      lpSize->cx = 0;
      lpSize->cy = 0;
      return FALSE;
   }

   lpSize->cx = FTInfo.x;
   lpSize->cy = FTInfo.pftCache->GetTextMetric(hdc).tmHeight;
   return TRUE;
}

BOOL FreeTypeGetCharWidth(const HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer)
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if (pSettings->WidthMode() == SETTING_WIDTHMODE_GDI32) {
      return ORIG_GetCharWidth32W(hdc, iFirstChar, iLastChar, lpBuffer);
   }

   CCriticalSectionLock __lock;

   if (!lpBuffer || iFirstChar > iLastChar || iFirstChar > 0xffff || iLastChar > 0xffff) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   //とりあえず0埋め
   ZeroMemory(lpBuffer, sizeof(INT) * (iLastChar - iFirstChar));

   LOGFONT lf;
   if (!GetLogFontFromDC(hdc, lf))
      return FALSE;

   FREETYPE_PARAMS params(0, hdc);
   FreeTypeDrawInfo FTInfo(params, hdc, &lf);
   if(!FreeTypePrepare(FTInfo))
      return FALSE;

   WCHAR wch = (WORD)iFirstChar;
   const WCHAR wend = (WORD)iLastChar;

   while (wch <= wend) {
      FTInfo.x = 0;
      if(!ForEachGetGlyph(FTInfo, &wch, 1, GetTextExtentCallback))
         return FALSE;
      *lpBuffer++ = FTInfo.x;
      wch++;
   }
   return TRUE;
}

void VertFinalizer(void *object){
   FT_Face face = (FT_Face)object;
   ft2vert_final(face, (struct ft2vert_st *)face->generic.data);
}

//
// グリフをIVSで指定された字形をサポートするかどうか調べ、
// サポートしている場合はグリフを置換する。
// サポートしていなければ何もしない。
//
void FreeTypeSubstGlyph(const HDC hdc, 
   const WORD vsindex,
   const int baseChar,
   int cChars, 
   SCRIPT_ANALYSIS* psa, 
   WORD* pwOutGlyphs, 
   WORD* pwLogClust, 
   SCRIPT_VISATTR* psva, 
   int* pcGlyphs 
   )
{
   CThreadLocalInfo* pTLInfo = g_TLInfo.GetPtr();
   CBitmapCache& cache = pTLInfo->BitmapCache();
   CCriticalSectionLock __lock;

   LOGFONT lf;
   if (!GetLogFontFromDC(hdc, lf))
      return;

   FREETYPE_PARAMS params(0, hdc);
   FreeTypeDrawInfo FTInfo(params, hdc, &lf, &cache);
   if(!FreeTypePrepare(FTInfo))
      return;

   FT_UInt glyph_index = ft2_subst_uvs(FTInfo.freetype_face, pwOutGlyphs[*pcGlyphs - 1], vsindex, baseChar);
   if (glyph_index) {
      pwOutGlyphs[*pcGlyphs - 1] = glyph_index; // 置換を実行
      // ASCII空白のグリフを取得
      glyph_index = FTC_CMapCache_Lookup(
         cmap_cache,
         FTInfo.font_type.face_id,
         FTInfo.cmap_index,
         ' ');
      // ゼロ幅グリフにする
      pwOutGlyphs[*pcGlyphs] = glyph_index;
      psva[*pcGlyphs].uJustification = SCRIPT_JUSTIFY_NONE;
      psva[*pcGlyphs].fClusterStart = 0;
      psva[*pcGlyphs].fDiacritic = 0;
      psva[*pcGlyphs].fZeroWidth = 1;
      psva[*pcGlyphs].fReserved = 0;
      psva[*pcGlyphs].fShapeReserved = 0;
   } else {
      // フォントは指定された字形を持たない。IVSのグリフを取得
      glyph_index = FTC_CMapCache_Lookup(
         cmap_cache,
         FTInfo.font_type.face_id,
         FTInfo.cmap_index,
         vsindex + 0xE0100);
      // IVSをサポートしていないフォントはIVSのグリフを持っている可能性もほとんどない。
      // missing glyphを返すとフォールバックされてしまうため確実に持っていそうなグリフを拾う
      if (!glyph_index)
         glyph_index = FTC_CMapCache_Lookup(
            cmap_cache,
            FTInfo.font_type.face_id,
            FTInfo.cmap_index,
            0x30FB);
      pwOutGlyphs[*pcGlyphs] = glyph_index;
      psva[*pcGlyphs] = psva[*pcGlyphs - 1];
      psva[*pcGlyphs].fClusterStart = 0;
   }
   pwLogClust[cChars - 2] = *pcGlyphs;
   pwLogClust[cChars - 1] = *pcGlyphs;
   ++*pcGlyphs;
}

FT_Error face_requester(
      FTC_FaceID face_id,
      FT_Library /*library*/,
      FT_Pointer /*request_data*/,
      FT_Face* aface)
{
   FT_Error ret;
   FT_Face face;

   FreeTypeFontInfo* pfi = g_pFTEngine->FindFont((int)face_id);
   Assert(pfi);
   if (!pfi) {
      return FT_Err_Invalid_Argument;
   }
   LPCTSTR fontname = pfi->GetName();

   // 名称を指定してフォントを取得
   FreeTypeSysFontData* pData = FreeTypeSysFontData::CreateInstance(fontname, pfi->GetFontWeight(), pfi->IsItalic());
   if(pData == NULL){
      return FT_Err_Cannot_Open_Resource;
   }

   face = pData->GetFace();
   Assert(face != NULL);

   // Charmapを設定しておく
   ret = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
   if(ret != FT_Err_Ok){
      FT_Done_Face(face);
      return ret;
   }

   // グリフ置換のため常に必要
   struct ft2vert_st *vert = ft2vert_init(face);
   face->generic.data = vert;
   face->generic.finalizer = VertFinalizer;

   *aface = face;
   return 0;
}


/*
DWORD FreeTypeGetVersion()
{
   int major = 0, minor = 0, patch = 0;
   FT_Library_Version(freetype_library, &major, &minor, &patch);
   //面倒なのでRGBマクロ使用
   return RGB(major, minor, patch);
}*/


//新太字アルゴリズム
FT_Error New_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos str_h, FT_Pos str_v )
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   int orientation = 0;
   switch (pSettings->BolderMode()) {
   case 1:
      return Old_FT_Outline_Embolden( outline, str_h );

   case 2:
      return FT_Outline_Embolden( outline, str_h );

   default:
      {
         if ( !outline ) return FT_Err_Invalid_Argument;
         orientation = FT_Outline_Get_Orientation( outline );
         if ( orientation == FT_ORIENTATION_NONE )
            if ( outline->n_contours ) return FT_Err_Invalid_Argument;
         Vert_FT_Outline_Embolden( outline, str_v );
         Old_FT_Outline_Embolden( outline, str_h );
         return FT_Err_Ok;
      }
   }
}

//横方向だけ太らせるFT_Outline_Embolden
FT_Error Old_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength )
{
   FT_Vector*  points;
   FT_Vector   v_prev, v_first, v_next, v_cur;
   FT_Angle rotate, angle_in, angle_out;
   FT_Int      c, n, first;
   FT_Int      orientation;

   if ( !outline )
      return FT_Err_Invalid_Argument;

   strength /= 2;
   if ( strength == 0 )
      return FT_Err_Ok;

   orientation = FT_Outline_Get_Orientation( outline );
   if ( orientation == FT_ORIENTATION_NONE )
   {
      if ( outline->n_contours )
         return FT_Err_Invalid_Argument;
      else
         return FT_Err_Ok;
   }

   if ( orientation == FT_ORIENTATION_TRUETYPE )
      rotate = -FT_ANGLE_PI2;
   else
      rotate = FT_ANGLE_PI2;

   points = outline->points;

   first = 0;
   for ( c = 0; c < outline->n_contours; c++ )
   {
      int  last = outline->contours[c];

      v_first = points[first];
      v_prev  = points[last];
      v_cur   = v_first;

      for ( n = first; n <= last; n++ )
      {
         FT_Vector   in, out;
         FT_Angle angle_diff;
         FT_Pos      d;
         FT_Fixed scale;

         if ( n < last )
            v_next = points[n + 1];
         else
            v_next = v_first;

         /* compute the in and out vectors */
         in.x = v_cur.x - v_prev.x;
         in.y = v_cur.y - v_prev.y;

         out.x = v_next.x - v_cur.x;
         out.y = v_next.y - v_cur.y;

         angle_in   = FT_Atan2( in.x, in.y );
         angle_out  = FT_Atan2( out.x, out.y );
         angle_diff = FT_Angle_Diff( angle_in, angle_out );
         scale      = FT_Cos( angle_diff / 2 );

         if ( scale < 0x4000L && scale > -0x4000L )
            in.x = in.y = 0;
         else
         {
            d = FT_DivFix( strength, scale );

            FT_Vector_From_Polar( &in, d, angle_in + angle_diff / 2 - rotate );
         }

         outline->points[n].x = v_cur.x + strength + in.x;
         //↓これをコメントアウトしただけ
         //outline->points[n].y = v_cur.y + strength + in.y;

         v_prev = v_cur;
         v_cur  = v_next;
      }

      first = last + 1;
   }

   return FT_Err_Ok;
}

//こっちは縦方向
FT_Error Vert_FT_Outline_Embolden( FT_Outline*  outline, FT_Pos strength )
{
   FT_Vector*  points;
   FT_Vector   v_prev, v_first, v_next, v_cur;
   FT_Angle rotate, angle_in, angle_out;
   FT_Int      c, n, first;
   FT_Int      orientation;

   if ( !outline )
      return FT_Err_Invalid_Argument;

   strength /= 2;
   if ( strength == 0 )
      return FT_Err_Ok;

   orientation = FT_Outline_Get_Orientation( outline );
   if ( orientation == FT_ORIENTATION_NONE )
   {
      if ( outline->n_contours )
         return FT_Err_Invalid_Argument;
      else
         return FT_Err_Ok;
   }

   if ( orientation == FT_ORIENTATION_TRUETYPE )
      rotate = -FT_ANGLE_PI2;
   else
      rotate = FT_ANGLE_PI2;

   points = outline->points;

   first = 0;
   for ( c = 0; c < outline->n_contours; c++ )
   {
      int  last = outline->contours[c];

      v_first = points[first];
      v_prev  = points[last];
      v_cur   = v_first;

      for ( n = first; n <= last; n++ )
      {
         FT_Vector   in, out;
         FT_Angle angle_diff;
         FT_Pos      d;
         FT_Fixed scale;

         if ( n < last )
            v_next = points[n + 1];
         else
            v_next = v_first;

         /* compute the in and out vectors */
         in.x = v_cur.x - v_prev.x;
         in.y = v_cur.y - v_prev.y;

         out.x = v_next.x - v_cur.x;
         out.y = v_next.y - v_cur.y;

         angle_in   = FT_Atan2( in.x, in.y );
         angle_out  = FT_Atan2( out.x, out.y );
         angle_diff = FT_Angle_Diff( angle_in, angle_out );
         scale      = FT_Cos( angle_diff / 2 );

         if ( scale < 0x4000L && scale > -0x4000L )
            in.x = in.y = 0;
         else
         {
            d = FT_DivFix( strength, scale );

            FT_Vector_From_Polar( &in, d, angle_in + angle_diff / 2 - rotate );
         }

         //outline->points[n].x = v_cur.x + strength + in.x;
         //↑これをコメントアウトしただけ
         outline->points[n].y = v_cur.y + strength + in.y;

         v_prev = v_cur;
         v_cur  = v_next;
      }

      first = last + 1;
   }

   return FT_Err_Ok;
}

//ダミー
FT_EXPORT( FT_Error )
FT_Library_SetLcdFilter_Dummy( FT_Library    /*library*/,
                               FT_LcdFilter  /*filter*/ )
{
   return 0;
}

BOOL FontLInit(void){
   CCriticalSectionLock __lock;

   if(FT_Init_FreeType(&freetype_library)){
      return FALSE;
   }
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   if(FTC_Manager_New(freetype_library,
      pSettings->CacheMaxFaces(),
      pSettings->CacheMaxSizes(),
      pSettings->CacheMaxBytes(),
      face_requester, NULL,
      &cache_man))
   {
      FontLFree();
      return FALSE;
   }
   if(FTC_CMapCache_New(cache_man, &cmap_cache)){
      FontLFree();
      return FALSE;
   }
   if(FTC_ImageCache_New(cache_man, &image_cache)){
      FontLFree();
      return FALSE;
   }

   const int nLcdFilter = pSettings->LcdFilter();
   if ((int)FT_LCD_FILTER_NONE < nLcdFilter && nLcdFilter < (int)FT_LCD_FILTER_MAX) {
      FT_Library_SetLcdFilter(freetype_library, (FT_LcdFilter)nLcdFilter);
   }

   s_AlphaBlendTable.init();
   return TRUE;
}

void FontLFree(void){
   CCriticalSectionLock __lock;

   if(cache_man != NULL)
      FTC_Manager_Done(cache_man);
   if(freetype_library != NULL)
      FT_Done_FreeType(freetype_library);
   cache_man = NULL;
   freetype_library = NULL;
}
