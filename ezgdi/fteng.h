#pragma once

class FreeTypeFontEngine;
extern FreeTypeFontEngine* g_pFTEngine;

enum FT_EngineConstants {
   FT_MAX_CHARS   = 65536,
};

/*
  FreeTypeに文字幅、太字、斜体をキャッシュする機構が無いのでそれらを補う

  1. まずDllMain(DLL_PROCESS_ATTACH)でFreeTypeFontEngineのインスタンスが生成される。
     (順番はCGdiPPSettings→FontLInit(FreeType)→FreeTypeFontEngine→フック)
     ForceChangeFontもここで処理する。

  2. CreateFontでFreeTypeFontEngine::AddFontが呼び出され、FreeTypeFontInfoと
     フォント名を結びつける。
     ついでにFreeTypeFontInfoはIndividualの設定をコピーして持つ。

  3. ExtTextOutやGetTextExtentなどからFreeTypePrepare関数が呼び出されると
     さらに内部でFreeTypeFontInfo::GetCacheが呼び出され、フォントサイズなどから
     FreeTypeFontCacheを得る。無ければ生成する。
     FreeTypeFontCacheは内部にFreeTypeCharDataのテーブル(UCS2なので2^16個)を
     持っていて、FreeTypeCharDataには文字毎にキャッシュデータを保管する。

  4. FreeTypeFontCacheから、文字またはグリフ番号を元にFreeTypeCharDataを得る。
     キャッシュがあれば(メモリ中に残っていれば)、MRUカウンタをセットする。
     無い場合は一旦スルーし、後でAddCharDataでキャッシュを追加する。

  5. 追加しまくるとメモリを喰らうので、追加が一定数(FREETYPE_REQCOUNTMAX)を超えると
     GCモドキで最近参照されたキャッシュデータをFREETYPE_GC_COUNTER個だけ残し、
     それ以外のデータ(FreeTypeCharData)は開放される。
     この2つの定数はiniで設定変更できた方がいいような気もする。

  6. 最後に、DllMain(DLL_PROCESS_DETACH)でFreeTypeFontEngineのインスタンスが破棄され、
     全てのキャッシュメモリが開放される。

 */

class FreeTypeGCCounter
{
private:
   int m_addcount;      //追加用
   int m_mrucount;      //MRU用

public:
   FreeTypeGCCounter()
      : m_addcount(0), m_mrucount(0)
   {
   }
   int AddIncrement() { return ++m_addcount; }
   int MruIncrement() { return ++m_mrucount; }

   void ResetGCCounter()
   {
      m_mrucount = 0;
      m_addcount = 0;
   }
};

class FreeTypeMruCounter
{
private:
   int m_mrucounter; //GC用

public:
   FreeTypeMruCounter(int n)
      : m_mrucounter(n)
   {
   }

   //GC用MRUカウンタ
   int GetMruCounter() const { return m_mrucounter; }
   void ResetMruCounter() { m_mrucounter = 0; }
   void SetMruCounter(FreeTypeGCCounter* p) { m_mrucounter = p->MruIncrement(); }
};

//文字幅、(glyph index)、FT_BitmapGlyph(太字、斜体のみ)をキャッシュする
class FreeTypeCharData : public FreeTypeMruCounter
{
private:
   typedef CValArray<FreeTypeCharData**>  CharDataArray;
   CharDataArray     m_arrSelfChar; //自分自身の保存元(Char)
   FreeTypeCharData**   m_ppSelfGlyph; //(Glyph)
   UINT           m_glyphindex;  //グリフ番号
   int               m_width;    //文字幅
   FT_BitmapGlyph    m_glyph;    //カラー用
   FT_BitmapGlyph    m_glyphMono;   //モノクロ用
   int               m_bmpSize;     //ビットマップサイズ
   int               m_bmpMonoSize; // 〃
// LONG           m_refcounter;  //参照カウンタ

   NOCOPY(FreeTypeCharData);

   //FT_Bitmap::bufferのサイズを返す
   static inline int FT_Bitmap_CalcSize(FT_BitmapGlyph gl)
   {
      return gl->bitmap.pitch * gl->bitmap.rows;
   }

public:
   FreeTypeCharData(FreeTypeCharData** ppCh, FreeTypeCharData** ppGl, WCHAR wch, UINT glyphindex, int width, int mru);
   ~FreeTypeCharData();

   WCHAR GetChar() const { return L'?'; }
   UINT GetGlyphIndex() const { return m_glyphindex; }
   int GetWidth() const { return m_width; }

   void AddChar(FreeTypeCharData** ppCh)
   {
      if (ppCh)
         m_arrSelfChar.Add(ppCh);
   }
   FT_BitmapGlyph GetGlyph(FT_Render_Mode render_mode) const
   {
      return (render_mode == FT_RENDER_MODE_MONO) ? m_glyphMono : m_glyph;
   }
   void SetGlyph(FT_Render_Mode render_mode, FT_BitmapGlyph glyph);

   void Erase()
   {
      delete this;
   }
};

class FreeTypeFontCache : public FreeTypeMruCounter, public FreeTypeGCCounter
{
private:
   int  m_px;
   int  m_weight;
   bool m_italic;
   bool m_active;
   TEXTMETRIC m_tm;

   //4×65536×2＝512KBぐらいたかが知れてるので固定配列で問題無し
   FreeTypeCharData* m_chars[FT_MAX_CHARS];
   FreeTypeCharData* m_glyphs[FT_MAX_CHARS];

   NOCOPY(FreeTypeFontCache);
   void Compact();

   FreeTypeCharData** _GetChar(WCHAR wch)
   {
      return m_chars + wch;
   }
   FreeTypeCharData** _GetGlyph(UINT glyph)
   {
      return m_glyphs + glyph;
   }

public:
   FreeTypeFontCache(int px, int weight, bool italic, int mru);
   ~FreeTypeFontCache();

   const TEXTMETRIC& GetTextMetric(HDC hdc)
   {
      if (m_tm.tmHeight == 0) {
         ::GetTextMetrics(hdc, &m_tm);
      }
      return m_tm;
   }

   bool Equals(int px, int weight, bool italic) const
   {
      return (m_px == px && m_weight == weight && m_italic == italic);
   }
   FreeTypeCharData* FindChar(WCHAR wch)
   {
      FreeTypeCharData* p = *_GetChar(wch);
      if(p) {
         p->SetMruCounter(this);
      }
      return p;
   }

   FreeTypeCharData* FindGlyphIndex(UINT glyph)
   {
      FreeTypeCharData* p = (glyph & 0xffff0000) ? NULL : *_GetGlyph(glyph);
      if(p) {
         p->SetMruCounter(this);
      }
      return p;
   }

   bool Activate()
   {
      if (!m_active) {
         m_active = true;
         return true;
      }
      return false;
   }

   void Erase();
   void AddCharData(WCHAR wch, UINT glyphindex, int width, FT_BitmapGlyph glyph, FT_Render_Mode render_mode);
   void AddGlyphData(UINT glyphindex, int width, FT_BitmapGlyph glyph, FT_Render_Mode render_mode);
};


// フォント名とFaceID(intを使うことにする)
class FreeTypeFontInfo : public FreeTypeMruCounter, public FreeTypeGCCounter
{
private:
   int  m_id;
   int  m_weight;
   bool m_italic;
   int  m_ftWeight;
   int  m_nMaxSizes;
   CFontSettings m_set;
   StringHashFont m_hash;
   typedef CArray<FreeTypeFontCache*>  CacheArray;
   CacheArray m_cache;

   NOCOPY(FreeTypeFontInfo);
   void Compact();

public:
	int GetCacheSize() const { return m_cache.GetSize(); }

   FreeTypeFontInfo(int n, LPCTSTR name, int weight, bool italic, const CFontSettings& set, int mru)
      : m_id(n), m_weight(weight), m_italic(italic), m_hash(name)
      , FreeTypeMruCounter(mru)
   {
      m_set = set;
      m_ftWeight = CalcBoldWeight(weight);

      enum { FTC_MAX_SIZES_DEFAULT = 4 };
      const CGdippSettings* pSettings = CGdippSettings::GetInstance();
      m_nMaxSizes = pSettings->CacheMaxSizes();
      if (!m_nMaxSizes)
         m_nMaxSizes = FTC_MAX_SIZES_DEFAULT;
   }
   ~FreeTypeFontInfo()
   {
      Erase();
   }

   int CalcNormalWeight() const
   {
      return m_set.GetNormalWeight();
   }
   int CalcBoldWeight(int weight) const
   {
//    return weight - FW_NORMAL) / 8;
//    return ((weight - FW_NORMAL) / 12) + (m_set.GetBoldWeight() << 2);
      if (weight <= FW_NORMAL) {
         return 0;
      }
      return ((weight - FW_NORMAL) / 8) + (m_set.GetBoldWeight() << 2);
   }
   int CalcBoldWeight(const LOGFONT& lf) const
   {
      return CalcBoldWeight(lf.lfWeight);
   }
   void CalcItalicSlant(FT_Matrix& matrix) const
   {
      matrix.xx = 1 << 16;
//    matrix.xy = 0x5800;
      matrix.xy = (5 + m_set.GetItalicSlant()) << 12;
      matrix.yx = 0;
      matrix.yy = 1 << 16;
   }

   bool Equals(const StringHashFont& hash, int weight, bool italic) const
   {
      weight = CalcBoldWeight(weight);
      return (m_ftWeight == weight && m_italic == italic && m_hash == hash);
   }

   int GetId() const { return m_id; }
   LPCTSTR GetName() const { return m_hash.c_str(); }
   int GetFontWeight() const { return m_weight; }
   int GetFTWeight() const { return m_ftWeight; }
   bool IsItalic() const { return m_italic; }
   const StringHashFont& GetHash() const { return m_hash; }

   const CFontSettings& GetFontSettings() const { return m_set; }
   bool operator ==(const FreeTypeFontInfo& x) const { return (m_hash == x.m_hash); }

   FreeTypeFontCache* GetCache(int px, const LOGFONT& lf);

   void Erase()
   {
      CArray<FreeTypeFontCache*>& arr = m_cache;
      int n = arr.GetSize();
      while (n-- > 0) {
         delete arr[n];
         arr[n] = NULL;
      }
      arr.RemoveAll();
   }
};

class FreeTypeFontEngine : public FreeTypeGCCounter
{
private:
   typedef CArray<FreeTypeFontInfo*>   FontListArray;
   FontListArray  m_arrFontList;
   int            m_nMaxFaces;
   int            m_nMemUsed;
   int             m_nId;

   void Compact();

public:
   FreeTypeFontEngine()
      : m_nMemUsed(0), m_nMaxFaces(0)
   {
      enum { FTC_MAX_FACES_DEFAULT = 2 };
      const CGdippSettings* pSettings = CGdippSettings::GetInstanceNoInit();
      m_nMaxFaces = pSettings->CacheMaxFaces();
      if (m_nMaxFaces == 0)
         m_nMaxFaces = FTC_MAX_FACES_DEFAULT;
      m_nId = 0;
   }
   ~FreeTypeFontEngine()
   {
      FontListArray& arr = m_arrFontList;
      int n = arr.GetSize();
      while (n--)
         delete arr[n];
   }

   bool AddFont(LPCTSTR lpFaceName, int weight, bool italic);
   int  GetFontIdByName(LPCTSTR lpFaceName, int weight, bool italic);
// LPCTSTR GetFontById(int faceid, int& weight, bool& italic);
   FreeTypeFontInfo* FindFont(LPCTSTR lpFaceName, int weight, bool italic);
   FreeTypeFontInfo* FindFont(int faceid);

   bool FontExists(LPCTSTR lpFaceName, int weight, bool italic)
   {
      return !!FindFont(lpFaceName, weight, italic);
   }

   //メモリ使用量カウンタ
   void AddMemUsed(int x)
   {
      m_nMemUsed += x;
   }
   void SubMemUsed(int x)
   {
      m_nMemUsed -= x;
      if (m_nMemUsed < 0)
         m_nMemUsed = 0;
   }
   template <class T>
   void AddMemUsedObj(T* /*p*/)
   {
      AddMemUsed(sizeof(T));
   }
   template <class T>
   void SubMemUsedObj(T* /*p*/)
   {
      SubMemUsed(sizeof(T));
   }
   int GetFreeId()
   {
      return ++m_nId;
   }
};


//GetFontDataのメモリストリーム
class FreeTypeSysFontData
{
private:
   HDC      m_hdc;
   HFONT m_hOldFont;
   bool  m_isTTC;
   bool  m_locked;
   void* m_pMapping;
   DWORD m_dwSize;
   FT_Face  m_ftFace;
   FT_StreamRec m_ftStream;

   FreeTypeSysFontData()
      : m_hdc(NULL)
      , m_hOldFont(NULL)
      , m_isTTC(false)
      , m_locked(false)
      , m_pMapping(NULL)
      , m_dwSize(0)
      , m_ftFace(NULL)
   {
      ZeroMemory(&m_ftStream, sizeof(FT_StreamRec));
   }

   static unsigned long IoFunc(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count);
   static void CloseFunc(FT_Stream  stream);
   bool OpenFaceByIndex(int index);
   bool Init(LPCTSTR name, int weight, bool italic);

public:
   static FreeTypeSysFontData* CreateInstance(LPCTSTR name, int weight, bool italic);
   ~FreeTypeSysFontData()
   {
      if (m_pMapping) {
         UnmapViewOfFile(m_pMapping);
      }
      if (m_hOldFont) {
         DeleteFont(SelectFont(m_hdc, m_hOldFont));
      }
      if (m_hdc) {
         DeleteDC(m_hdc);
      }
   }

   FT_Face GetFace()
   {
      FT_Face face = m_ftFace;
      m_ftFace = NULL;
      return face;
   }
};
