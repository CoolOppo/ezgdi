#include "stdafx.h"
#include "settings.h"
#include "ft.h"
#include "fteng.h"

#define FREETYPE_REQCOUNTMAX  4096
#define FREETYPE_GC_COUNTER      1024

FreeTypeFontEngine* g_pFTEngine;
FT_Library     freetype_library;
FTC_Manager    cache_man;
FTC_CMapCache  cmap_cache;
FTC_ImageCache image_cache;

template <class T>
struct GCCounterSortFunc : public std::binary_function<const T*, const T*, bool>
{
   bool operator()(const T* arg1, const T* arg2) const
   {
      const int cnt1 = arg1 ? arg1->GetMruCounter() : -1;
      const int cnt2 = arg2 ? arg2->GetMruCounter() : -1;
      return cnt1 > cnt2;
   }
};

struct DeleteCharFunc : public std::unary_function<FreeTypeCharData*&, void>
{
   void operator()(FreeTypeCharData*& arg) const
   {
      if (!arg)
         return;
      delete arg;
      arg = NULL;
   }
};

template <class T>
void Compact(T** pp, int count, int reduce)
{
   Assert(count >= 0);
   Assert(reduce > 0);
   if (!pp || !count || count < reduce) {
      return;
   }

   T** ppTemp = (T**)malloc(sizeof(T*) * count);
   if (!ppTemp) {
      return;
   }
   memcpy(ppTemp, pp, sizeof(T*) * count);

   std::sort(ppTemp, ppTemp + count, GCCounterSortFunc<T>());
   int i;
   for (i=0; i<reduce; i++) {
      if (!ppTemp[i])
         break;
      ppTemp[i]->ResetMruCounter();
   }

   for (i=reduce; i<count; i++) {
      if (!ppTemp[i])
         break;
      ppTemp[i]->Erase();
   }
   free(ppTemp);
}

//FreeTypeCharData
FreeTypeCharData::FreeTypeCharData(FreeTypeCharData** ppCh, FreeTypeCharData** ppGl, WCHAR wch, UINT glyphindex, int width, int mru)
   : m_ppSelfGlyph(ppGl), m_glyphindex(glyphindex), m_width(width)
   , m_glyph(NULL), m_glyphMono(NULL), m_bmpSize(0), m_bmpMonoSize(0)
   , FreeTypeMruCounter(mru)
{
   g_pFTEngine->AddMemUsedObj(this);
   AddChar(ppCh);
}

FreeTypeCharData::~FreeTypeCharData()
{
   CharDataArray& arr = m_arrSelfChar;
   int n = arr.GetSize();
   while (n--) {
      FreeTypeCharData** pp = arr[n];
      Assert(*pp == this);
      *pp = NULL;
   }
   if(m_ppSelfGlyph) {
      Assert(*m_ppSelfGlyph == this);
      *m_ppSelfGlyph = NULL;
   }
   if(m_glyph){
      FT_Done_Glyph((FT_Glyph)m_glyph);
   }
   if(m_glyphMono){
      FT_Done_Glyph((FT_Glyph)m_glyphMono);
   }

   g_pFTEngine->SubMemUsed(m_bmpSize);
   g_pFTEngine->SubMemUsed(m_bmpMonoSize);
   g_pFTEngine->SubMemUsedObj(this);
}

void FreeTypeCharData::SetGlyph(FT_Render_Mode render_mode, FT_BitmapGlyph glyph)
{
   const bool bMono = (render_mode == FT_RENDER_MODE_MONO);
   FT_BitmapGlyph& gl = bMono ? m_glyphMono : m_glyph;
   if (!gl) {
      FT_Glyph_Copy((FT_Glyph)glyph, (FT_Glyph*)&gl);
      if (gl) {
         int& size = bMono ? m_bmpMonoSize : m_bmpSize;
         size  = FT_Bitmap_CalcSize(gl);
         size += sizeof(FT_BitmapGlyphRec);
         g_pFTEngine->AddMemUsed(size);
      }
   }
}


//FreeTypeFontCache
FreeTypeFontCache::FreeTypeFontCache(int px, int weight, bool italic, int mru)
   : m_px(px), m_weight(weight), m_italic(italic), m_active(false)
   , FreeTypeMruCounter(mru)
{
   ZeroMemory(&m_tm, sizeof(TEXTMETRIC));
   ZeroMemory(m_chars, sizeof(m_chars));
   ZeroMemory(m_glyphs, sizeof(m_glyphs));
   g_pFTEngine->AddMemUsedObj(this);
}

FreeTypeFontCache::~FreeTypeFontCache()
{
   Erase();
   g_pFTEngine->SubMemUsedObj(this);
}

void FreeTypeFontCache::Compact()
{
   ResetGCCounter();
   ::Compact(m_glyphs, countof(m_glyphs), FREETYPE_GC_COUNTER);
}

void FreeTypeFontCache::Erase()
{
   m_active = false;
   std::for_each(m_chars,  m_chars  + FT_MAX_CHARS, DeleteCharFunc());
   std::for_each(m_glyphs, m_glyphs + FT_MAX_CHARS, DeleteCharFunc());
}

void FreeTypeFontCache::AddCharData(WCHAR wch, UINT glyphindex, int width, FT_BitmapGlyph glyph, FT_Render_Mode render_mode)
{
   if (glyphindex & 0xffff0000) {
      return;
   }
   
   FreeTypeCharData** ppChar  = _GetChar(wch);
   FreeTypeCharData** ppGlyph = _GetGlyph(glyphindex);

   Assert(!(*ppChar && !*ppGlyph));

   //既にあればMRUカウンタを上げる
   if (*ppChar) {
      (*ppChar)->SetGlyph(render_mode, glyph);
      (*ppChar)->SetMruCounter(this);
      return;
   } else if (*ppGlyph) {
      *ppChar = *ppGlyph;
      (*ppGlyph)->AddChar(ppChar);
      (*ppGlyph)->SetGlyph(render_mode, glyph);
      (*ppGlyph)->SetMruCounter(this);
      return;
   }

   if (AddIncrement() >= FREETYPE_REQCOUNTMAX) {
      Compact();
   }

   //追加(文字+glyph)
   FreeTypeCharData* p = new FreeTypeCharData(ppChar, ppGlyph, wch, glyphindex, width, MruIncrement());
   if (p == NULL) {
      return;
   }
   p->SetGlyph(render_mode, glyph);
   *ppChar = *ppGlyph = p;
}

void FreeTypeFontCache::AddGlyphData(UINT glyphindex, int width, FT_BitmapGlyph glyph, FT_Render_Mode render_mode)
{
   if (glyphindex & 0xffff0000) {
      return;
   }

   FreeTypeCharData** ppGlyph = _GetGlyph(glyphindex);
   //Assert(!!*ppChar == !!*ppGlyph);

   //既にあればMRUカウンタを上げる
   if (*ppGlyph) {
      (*ppGlyph)->SetGlyph(render_mode, glyph);
      (*ppGlyph)->SetMruCounter(this);
      return;
   }

   if (AddIncrement() >= FREETYPE_REQCOUNTMAX) {
      Compact();
   }

   //追加(glyphのみ)
   FreeTypeCharData* p = new FreeTypeCharData(NULL, ppGlyph, 0, glyphindex, width, MruIncrement());
   if (p == NULL) {
      return;
   }
   p->SetGlyph(render_mode, glyph);
   *ppGlyph = p;
}


//FreeTypeFontInfo
void FreeTypeFontInfo::Compact()
{
   ResetGCCounter();
   CacheArray& cache = m_cache;
   ::Compact(cache.GetData(), cache.GetSize(), m_nMaxSizes);
}

FreeTypeFontCache* FreeTypeFontInfo::GetCache(int px, const LOGFONT& lf)
{
   CacheArray& cache = m_cache;
   FreeTypeFontCache** pp  = cache.Begin();
   FreeTypeFontCache** end = cache.End();

   const int weight = lf.lfWeight;
   const bool italic = !!lf.lfItalic;
   FreeTypeFontCache* p = NULL;
   for(; pp != end; ++pp) {
      if ((*pp)->Equals(px, weight, italic)) {
         (*pp)->SetMruCounter(this);
         p = *pp;
         break;
      }
   }
   if (p) {
      goto OK;
   }

   p = new FreeTypeFontCache(px, weight, italic, MruIncrement());
   if (!p) {
      return NULL;
   }
   if (cache.Add(p)) {
      goto OK;
   }
   delete p;
   return NULL;

OK:
   Assert(p != NULL);
   if (p && p->Activate()) {
      if (AddIncrement() > m_nMaxSizes) {
         Compact();
      }
   }
   return p;
}


//FreeTypeFontEngine
void FreeTypeFontEngine::Compact()
{
   ResetGCCounter();
   FontListArray& arr = m_arrFontList;
   ::Compact(arr.GetData(), arr.GetSize(), m_nMaxFaces);
}

bool FreeTypeFontEngine::AddFont(LPCTSTR lpFaceName, int weight, bool italic)
{
   CCriticalSectionLock __lock;

   if(lpFaceName == NULL || _tcslen(lpFaceName) == 0 || FontExists(lpFaceName, weight, italic))
      return false;

   FontListArray& arr = m_arrFontList;
   if (AddIncrement() > m_nMaxFaces) {
      Compact();
   }

   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   const CFontSettings& fs = pSettings->FindIndividual(lpFaceName);
   FreeTypeFontInfo* pfi = new FreeTypeFontInfo(arr.GetSize() + 1, lpFaceName, weight, italic, fs, MruIncrement());
   if (!pfi)
      return false;

   bool ret = !!arr.Add(pfi);
   if (!ret) {
      delete pfi;
      return false;
   }

   return true;
}

int FreeTypeFontEngine::GetFontIdByName(LPCTSTR lpFaceName, int weight, bool italic)
{
   CCriticalSectionLock __lock;

   const FreeTypeFontInfo* pfi = FindFont(lpFaceName, weight, italic);
   return pfi ? pfi->GetId() : 0;
}

/*
LPCTSTR FreeTypeFontEngine::GetFontById(int faceid, int& weight, bool& italic)
{
   CCriticalSectionLock __lock;

   FreeTypeFontInfo** pp   = m_arrFontList.Begin();
   FreeTypeFontInfo** end  = m_arrFontList.End();
   for(; pp != end; ++pp) {
      FreeTypeFontInfo* p = *pp;
      if (p->GetId() == faceid) {
         p->SetMruCounter(this);
         weight = p->GetWeight();
         italic = p->IsItalic();
         return p->GetName();
      }
   }
   return NULL;
}
*/

FreeTypeFontInfo* FreeTypeFontEngine::FindFont(LPCTSTR lpFaceName, int weight, bool italic)
{
   CCriticalSectionLock __lock;
   FreeTypeFontInfo** pp   = m_arrFontList.Begin();
   FreeTypeFontInfo** end  = m_arrFontList.End();
   StringHashFont hash(lpFaceName);

   for(; pp != end; ++pp) {
      FreeTypeFontInfo* p = *pp;
      if (p->Equals(hash, weight, italic)) {
         p->SetMruCounter(this);
         return p;
      }
   }
   return NULL;
}

FreeTypeFontInfo* FreeTypeFontEngine::FindFont(int faceid)
{
   CCriticalSectionLock __lock;

   FreeTypeFontInfo** pp   = m_arrFontList.Begin();
   FreeTypeFontInfo** end  = m_arrFontList.End();
   for(; pp != end; ++pp) {
      FreeTypeFontInfo* p = *pp;
      if (p->GetId() == faceid) {
         p->SetMruCounter(this);
         return p;
      }
   }
   return NULL;
}


//FreeTypeSysFontData
// http://kikyou.info/diary/?200510#i10 を参考にした
#include <freetype/tttags.h>  // FT_TRUETYPE_TAGS_H
#include <freetype/tttables.h>   // FT_TRUETYPE_TABLES_H
#include <mmsystem.h>   //mmioFOURCC
#define TVP_TT_TABLE_ttcf  mmioFOURCC('t', 't', 'c', 'f')
#define TVP_TT_TABLE_name  mmioFOURCC('n', 'a', 'm', 'e')

// Windowsに登録されているフォントのバイナリデータを名称から取得
FreeTypeSysFontData* FreeTypeSysFontData::CreateInstance(LPCTSTR name, int weight, bool italic)
{
   FreeTypeSysFontData* pData = new FreeTypeSysFontData;
   if (!pData) {
      return NULL;
   }
   if (!pData->Init(name, weight, italic)) {
      delete pData;
      return NULL;
   }
   return pData;
}

bool FreeTypeSysFontData::Init(LPCTSTR name, int weight, bool italic)
{
   const CGdippSettings* pSettings = CGdippSettings::GetInstance();
   void* pNameFromGDI      = NULL; // Windows から取得した name タグの内容
   void* pNameFromFreeType = NULL; // FreeType から取得した name タグの内容
   HFONT hf = NULL;
   DWORD cbNameTable;
   DWORD cbFontData;
   int index;
   DWORD buf;
   FT_StreamRec& fsr = m_ftStream;

   m_hdc = CreateCompatibleDC(NULL);
   if(m_hdc == NULL) {
      return false;
   }
   // 名前以外適当
   hf = CreateFont(
            12, 0, 0, 0, weight,
            italic, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            name);
   if(hf == NULL){
      return false;
   }

   m_hOldFont = SelectFont(m_hdc, hf);
   // フォントデータが得られそうかチェック
   cbNameTable = ::GetFontData(m_hdc, TVP_TT_TABLE_name, 0, NULL, 0);
   if(cbNameTable == GDI_ERROR){
      goto ERROR_Init;
   }

   pNameFromGDI      = malloc(cbNameTable);
   if (!pNameFromGDI) {
      goto ERROR_Init;
   }
   pNameFromFreeType = malloc(cbNameTable);
   if (!pNameFromFreeType) {
      goto ERROR_Init;
   }

   //- name タグの内容をメモリに読み込む
   if(GetFontData(m_hdc, TVP_TT_TABLE_name, 0, pNameFromGDI, cbNameTable) == GDI_ERROR){
      goto ERROR_Init;
   }

   // フォントサイズ取得処理
   cbFontData = ::GetFontData(m_hdc, TVP_TT_TABLE_ttcf, 0, &buf, 1);
   if(cbFontData == 1){
      // TTC ファイルだと思われる
      cbFontData = ::GetFontData(m_hdc, TVP_TT_TABLE_ttcf, 0, NULL, 0);
      m_isTTC = true;
   }
   else{
      cbFontData = ::GetFontData(m_hdc, 0, 0, NULL, 0);
   }
   if(cbFontData == GDI_ERROR){
      // エラー; GetFontData では扱えなかった
      goto ERROR_Init;
   }

   if (pSettings->UseMapping()) {
      HANDLE hmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT | SEC_NOCACHE, 0, cbFontData, NULL);
      if (!hmap) {
         goto ERROR_Init;
      }
      m_pMapping = MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, cbFontData);
      m_dwSize = cbFontData;
      CloseHandle(hmap);

      if (m_pMapping) {
         ::GetFontData(m_hdc, m_isTTC ? TVP_TT_TABLE_ttcf : 0, 0, m_pMapping, cbFontData);
      }
   }

   // FT_StreamRec の各フィールドを埋める
   fsr.base          = 0;
   fsr.size          = cbFontData;
   fsr.pos              = 0;
   fsr.descriptor.pointer  = this;
   fsr.pathname.pointer = NULL;
   fsr.read          = IoFunc;
   fsr.close            = CloseFunc;

   index = 0;
   m_locked = true;
   if(!OpenFaceByIndex(index)){
      goto ERROR_Init;
   }

   for(;;) {
      // FreeType から、name タグのサイズを取得する
      FT_ULong length = 0;
      FT_Error err = FT_Load_Sfnt_Table(m_ftFace, TTAG_name, 0, NULL, &length);
      if(err){
         goto ERROR_Init;
      }

      // FreeType から得た name タグの長さを Windows から得た長さと比較
      if(length == cbNameTable){
         // FreeType から name タグを取得
         err = FT_Load_Sfnt_Table(m_ftFace, TTAG_name, 0, (unsigned char*)pNameFromFreeType, &length);
         if(err){
            goto ERROR_Init;
         }
         // FreeType から読み込んだ name タグの内容と、Windows から読み込んだ
         // name タグの内容を比較する。
         // 一致していればその index のフォントを使う。
         if(!memcmp(pNameFromGDI, pNameFromFreeType, cbNameTable)){
            // 一致した
            // face は開いたまま
            break; // ループを抜ける
         }
      }

      // 一致しなかった
      // インデックスを一つ増やし、その face を開く
      index ++;

      if(!OpenFaceByIndex(index)){
         // 一致する face がないまま インデックスが範囲を超えたと見られる
         // index を 0 に設定してその index を開き、ループを抜ける
         index = 0;
         if(!OpenFaceByIndex(index)){
            goto ERROR_Init;
         }
         break;
      }
   }

   free(pNameFromGDI);
   free(pNameFromFreeType);
   m_locked = false;
   return true;

ERROR_Init:
   m_locked = false;
   if (hf) {
      SelectFont(m_hdc, m_hOldFont);
      DeleteFont(hf);
      m_hOldFont = NULL;
   }
   free(pNameFromGDI);
   free(pNameFromFreeType);
   return false;
}

unsigned long FreeTypeSysFontData::IoFunc(
         FT_Stream      stream,
         unsigned long  offset,
         unsigned char* buffer,
         unsigned long  count )
{
   if(count == 0){
      return 0;
   }

   FreeTypeSysFontData * pThis = reinterpret_cast<FreeTypeSysFontData*>(stream->descriptor.pointer);
   Assert(pThis != NULL);

   DWORD result = 0;
   if (pThis->m_pMapping) {
      result = Min(pThis->m_dwSize - offset, count);
      memcpy(buffer, (const BYTE*)pThis->m_pMapping + offset, result);
   } else {
      result = ::GetFontData(pThis->m_hdc, pThis->m_isTTC ? TVP_TT_TABLE_ttcf : 0, offset, buffer, count);
      if(result == GDI_ERROR) {
         // エラー
         return 0;
      }
   }
   return result;
}

void FreeTypeSysFontData::CloseFunc(FT_Stream stream)
{
   FreeTypeSysFontData * pThis = reinterpret_cast<FreeTypeSysFontData*>(stream->descriptor.pointer);
   Assert(pThis != NULL);

   if(!pThis->m_locked)
      delete pThis;
}

bool FreeTypeSysFontData::OpenFaceByIndex(int index)
{
   if(m_ftFace) {
      FT_Done_Face(m_ftFace);
      m_ftFace = NULL;
   }

   FT_Open_Args args = { 0 };
   args.flags     = FT_OPEN_STREAM;
   args.stream    = &m_ftStream;

   // FreeType で扱えるか？
   return (FT_Open_Face(freetype_library, &args, index, &m_ftFace) == 0);
}
