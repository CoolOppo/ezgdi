#include "stdafx.h"

#include "settings.h"

EXTERN_C LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
   return CallNextHookEx(NULL, code, wParam, lParam);
}

EXTERN_C HRESULT WINAPI GdippDllGetVersion(DLLVERSIONINFO* pdvi)
{
   if (!pdvi || pdvi->cbSize < sizeof(DLLVERSIONINFO)) {
      return E_INVALIDARG;
   }

   const UINT cbSize = pdvi->cbSize;
   ZeroMemory(pdvi, cbSize);
   pdvi->cbSize = cbSize;

   HRSRC hRsrc = FindResource(GetDLLInstance(), MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
   if (!hRsrc) {
      return E_FAIL;
   }

   HGLOBAL hGlobal = LoadResource(GetDLLInstance(), hRsrc);
   if (!hGlobal) {
      return E_FAIL;
   }

   const WORD* lpwPtr = (const WORD*)LockResource(hGlobal);
   if (lpwPtr[1] != sizeof(VS_FIXEDFILEINFO)) {
      return E_FAIL;
   }

   const VS_FIXEDFILEINFO* pvffi = (const VS_FIXEDFILEINFO*)(lpwPtr + 20);
   if (pvffi->dwSignature != VS_FFI_SIGNATURE ||
         pvffi->dwStrucVersion != VS_FFI_STRUCVERSION) {
      return E_FAIL;
   }

   //8.0.2006.1027
   // -> Major: 8, Minor: 2006, Build: 1027
   pdvi->dwMajorVersion = HIWORD(pvffi->dwFileVersionMS);
   pdvi->dwMinorVersion = LOWORD(pvffi->dwFileVersionMS) * 10 + HIWORD(pvffi->dwFileVersionLS);
   pdvi->dwBuildNumber  = LOWORD(pvffi->dwFileVersionLS);
   pdvi->dwPlatformID   = DLLVER_PLATFORM_NT;

   if (pdvi->cbSize < sizeof(DLLVERSIONINFO2)) {
      return S_OK;
   }

   DLLVERSIONINFO2* pdvi2 = (DLLVERSIONINFO2*)pdvi;
   pdvi2->ullVersion      = MAKEDLLVERULL(pdvi->dwMajorVersion, pdvi->dwMinorVersion, pdvi->dwBuildNumber, 2);
   return S_OK;
}
