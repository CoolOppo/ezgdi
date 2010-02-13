#pragma once

#include <mmsystem.h> //mmioFOURCC
#define FOURCC_GDIPP mmioFOURCC('G', 'D', 'I', 'P')

typedef struct {
   int dummy;
   FOURCC magic;
// BYTE reserved[256];
} GDIPP_CREATE_MAGIC;

//éQè∆
//http://www.catch22.net/tuts/undoc01.asp

#ifdef _GDIPP_EXE
template <typename _STARTUPINFO>
void FillGdiPPStartupInfo(_STARTUPINFO& si, GDIPP_CREATE_MAGIC& gppcm)
{
   ZeroMemory(&gppcm, sizeof(GDIPP_CREATE_MAGIC));
   gppcm.magic = FOURCC_GDIPP;
   si.cbReserved2 = sizeof(GDIPP_CREATE_MAGIC);
   si.lpReserved2 = (LPBYTE)&gppcm;
}
#endif

#ifdef _GDIPP_DLL
template <typename _STARTUPINFO>
bool IsGdiPPStartupInfo(const _STARTUPINFO& si)
{
   if(si.cbReserved2 >= sizeof(int) + sizeof(FOURCC)) {
      GDIPP_CREATE_MAGIC* pMagic = (GDIPP_CREATE_MAGIC*)si.lpReserved2;
      if (pMagic->dummy == 0 && pMagic->magic == FOURCC_GDIPP) {
         return true;
      }
   }
   return false;
}
#endif
