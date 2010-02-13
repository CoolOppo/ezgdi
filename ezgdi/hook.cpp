#include "stdafx.h"

#include "override.h"
#include "ft.h"
#include "fteng.h"

#include <locale.h>

#ifdef USE_DETOURS
#include <detours.h>

/* define ORIG_* WIN32 API prototype */
#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   rettype (WINAPI * ORIG_##name) argtype;
#include "hooklist.h"
#undef HOOK_DEFINE

/* initialize ORIG_* with proc address */
static void hook_initinternal()
{
#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   ORIG_##name = name;
#include "hooklist.h"
#undef HOOK_DEFINE
}

/* install IMPL_* hooks to replace ORIG_* */
static void hook_init()
{
   DetourRestoreAfterWith();

   DetourTransactionBegin();
   DetourUpdateThread(GetCurrentThread());

#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   DetourAttach(&(PVOID&)ORIG_##name, IMPL_##name);
#include "hooklist.h"
#undef HOOK_DEFINE

   LONG error = DetourTransactionCommit();

   if (error != NOERROR) {
      TRACE(_T("hook_init error: %#x\n"), error);
   }
}

static void hook_term()
{
   DetourTransactionBegin();
   DetourUpdateThread(GetCurrentThread());

#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   DetourDetach(&(PVOID&)ORIG_##name, IMPL_##name);
#include "hooklist.h"
#undef HOOK_DEFINE

   LONG error = DetourTransactionCommit();

   if (error != NOERROR) {
      TRACE(_T("hook_term error: %#x\n"), error);
   }
}

#else /* EASYHOOK */

#include <easyhook.h>

#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   HOOK_TRACE_INFO HOOK_##name; \
   rettype (WINAPI * FUNC_##name) argtype = NULL; \
   rettype (WINAPI * ORIG_##name) argtype = NULL; \
   rettype WINAPI OLD_##name argtype { \
      ULONG ACLEntries[] = {0}; \
      LhSetExclusiveACL(ACLEntries, 1, &HOOK_##name); \
      rettype retval = ##name arglist; \
      LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name); \
      return retval; \
   }
#include "hooklist.h"
#undef HOOK_DEFINE

static FARPROC GetProcAddressInDll(LPCTSTR lpczDllName, LPCSTR lpczProcName)
{
   HMODULE hDLL = LoadLibrary(lpczDllName);
   if (hDLL == NULL)
      return NULL;
   FARPROC lpProc = GetProcAddress(hDLL, lpczProcName);
   FreeLibrary(hDLL);
   return lpProc;
}

static void hook_initinternal()
{
#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
   FUNC_##name = (rettype (WINAPI *) argtype) GetProcAddressInDll(_T(#modname) _T(".dll"), #name); \
   ORIG_##name = OLD_##name;
#include "hooklist.h"
#undef HOOK_DEFINE
}

#define FORCE(expr) {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}

static void hook_init()
{
   ULONG ACLEntries[1] = {0};
   NTSTATUS NtStatus;

#define HOOK_DEFINE(modname, name, rettype, argtype, arglist) \
    FORCE(LhInstallHook(FUNC_##name, IMPL_##name, (PVOID)0, &HOOK_##name));\
    FORCE(LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name));
#include "hooklist.h"
#undef HOOK_DEFINE

   FORCE(LhSetGlobalExclusiveACL(ACLEntries, 0));
   return;

ERROR_ABORT:
   TRACE(_T("hook_init error: %#x\n"), NtStatus);
   ;
}

static void hook_term()
{
   LhUninstallAllHooks();
   LhWaitForPendingRemovals();
}

#endif /* DETOURS and EASYHOOK */

HINSTANCE g_hinstDLL;
LONG      g_bHookEnabled;
CTlsData<CThreadLocalInfo> g_TLInfo;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID /*lp*/)
{
   CGdippSettings* pSettings;

   switch(reason) {
   case DLL_PROCESS_ATTACH:
      _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
      setlocale(LC_ALL, "");
      g_hinstDLL = instance;
      hook_initinternal();

      CCriticalSectionLock::Init();
      if (!g_TLInfo.ProcessInit()) {
         return FALSE;
      }

      pSettings = CGdippSettings::CreateInstance();
      if (!pSettings || !pSettings->LoadSettings(instance)) {
         CGdippSettings::DestroyInstance();
         return FALSE;
      }

      if (!IsProcessExcluded()) {
         if (!FontLInit())
            return FALSE;
         if ( !(g_pFTEngine = new FreeTypeFontEngine) )
            return FALSE;

         InterlockedExchange(&g_bHookEnabled, TRUE);
         hook_init();
      }

      break;
   case DLL_THREAD_ATTACH:
      break;
   case DLL_THREAD_DETACH:
      g_TLInfo.ThreadTerm();
      break;
   case DLL_PROCESS_DETACH:
      if (InterlockedExchange(&g_bHookEnabled, FALSE))
         hook_term();
      if (g_pFTEngine)
         delete g_pFTEngine;

      FontLFree();
      CGdippSettings::DestroyInstance();
      g_TLInfo.ProcessTerm();
      CCriticalSectionLock::Term();
      break;
   }
   return TRUE;
}
