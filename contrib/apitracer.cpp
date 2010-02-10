#define WINVER 0x500
#define WIN32_LEAN_AND_MEAN 1
#include <afx.h>
#include <usp10.h>

#include <easyhook.h>
#include <detours.h>
#include <set>
#include <algorithm>

enum {USE_NO_HOOKS, USE_EASYHOOK, USE_DETOURS, USE_HOOKS_MAX};

#define WINDOWS_KEY _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows")
#define HOOK_API _T("HookApi")

struct RuntimeError {
   LPCTSTR sWhat;
   LONG lRetCode;
   RuntimeError(LPCTSTR sWhatIn, LONG lRetCodeIn)
      : sWhat(sWhatIn), lRetCode(lRetCodeIn)
   { }
};

struct EasyHookError {
   DWORD dwRetCode;
   LPCTSTR lpczError;
   DWORD dwError;
   EasyHookError(DWORD dwRetCodeIn)
      : dwRetCode(dwRetCodeIn), lpczError(RtlGetLastErrorString()), dwError(RtlGetLastError())
   { }
};

struct RegKey {
   HKEY hKey;

   RegKey(HKEY hKeyRoot, LPCTSTR key, REGSAM rs) {
      LONG ret = RegOpenKeyEx(hKeyRoot, key, 0, rs, &hKey);
      if (ret != ERROR_SUCCESS)
         throw RuntimeError(_T("RegOpenKeyEx"), ret);
   }

   DWORD QueryValueDWORD(LPCTSTR lpValueName) {
      DWORD dwType, dwValue, dwLen;
      LONG ret = RegQueryValueEx(hKey, lpValueName, NULL, &dwType, (LPBYTE)&dwValue, &dwLen);
      if (ret != ERROR_SUCCESS)
         throw RuntimeError(_T("RegQueryValueEx"), ret);
      else if (dwType != REG_DWORD || dwLen != sizeof(DWORD))
         throw RuntimeError(_T("QueryValueDWORD not DWORD"), 0);
      return dwValue;
   }

   ~RegKey() {
      RegCloseKey(hKey);
   }
};

DWORD HookApiRegOption() 
{
   static DWORD dwValue = -1;
   if (dwValue != -1)
      return dwValue;

   try {
      RegKey rk(HKEY_LOCAL_MACHINE, WINDOWS_KEY, KEY_QUERY_VALUE);
      dwValue = rk.QueryValueDWORD(HOOK_API);
      if (dwValue < 0 || dwValue >= USE_HOOKS_MAX)
         dwValue = USE_NO_HOOKS;
      return dwValue;
   }
   catch(RuntimeError &error) {
      TRACE(_T("Error %s, return code %ld, last error: %d\n"), error.sWhat, error.lRetCode, GetLastError());
      return USE_NO_HOOKS;
   }
}

/* define easyhook hooks variable */
#define HOOK_DEFINE(rettype, name, argtype) \
            HOOK_TRACE_INFO EH_##name = {NULL};
#include "hooklist.h"
#undef HOOK_DEFINE

/* define detours original function pointer */
#define HOOK_DEFINE(rettype, name, argtype) \
            rettype (WINAPI * ORIG_##name) argtype;
#include "hooklist.h"
#undef HOOK_DEFINE

#define OPEN_ARGS(...) __VA_ARGS__

/* define hooked implementations */
#define HOOK_DEFINE(rettype, name, argtype, args, fmt, modname) \
            rettype WINAPI IMPL_##name argtype { \
               TRACE(_T("Thread 0x%x: ") _T(#name) _T("\t(") _T(fmt) _T(")\n"), GetCurrentThreadId(), OPEN_ARGS args); \
               return ORIG_##name args; \
            }
#include "hooklist2.h"
#undef HOOK_DEFINE

#define EH_FORCE(expr) do { \
   DWORD status = (expr); \
   if(!SUCCEEDED(status)) throw EasyHookError(status); \
} while(0)

struct DllModule {
   HMODULE hModule;

   DllModule(LPCTSTR lpczDllName) {
      if (NULL == (hModule = LoadLibrary(lpczDllName)))
         throw RuntimeError(_T("LoadLibrary"), (LONG)hModule);
   }
   ~DllModule() {
      FreeLibrary(hModule);
   }
};

FARPROC GetProcAddressInDll(LPCTSTR lpczDllName, LPCSTR lpczProcName)
{
   DllModule dll(lpczDllName);
   FARPROC lpProc = GetProcAddress(dll.hModule, lpczProcName);
   if (lpProc == NULL)
      throw RuntimeError(_T("GetProcAddress"), (LONG)lpProc);
   return lpProc;
}

ULONG ACLEntries[1] = {};

void HookInstall(DWORD dwHookApi)
{
   /* save old pointers */
   #define HOOK_DEFINE(rettype, name, argtype, args, fmt, modname) \
      ORIG_##name = (rettype (WINAPI *) argtype) GetProcAddressInDll(_T(modname), #name); \
      TRACE(_T("Function address: %ls::%ls (GetProcAddress: %p) (Raw: %p)\n"), _T(modname), _T(#name), ORIG_##name, ##name);
   #include "hooklist2.h"
   #undef HOOK_DEFINE

   if (dwHookApi == USE_EASYHOOK) {
      TRACE(_T("Install Easyhook Hooks\n"));
      DWORD status;
      BOOL flags;

      /* install easyhook hooks */
      #define HOOK_DEFINE(rettype, name, argtype) \
         TRACE(_T("Installing Hook %s\n"), _T(#name)); \
         EH_FORCE(LhInstallHook(ORIG_##name, IMPL_##name, 0, &EH_##name)); \
         EH_FORCE(LhSetExclusiveACL(ACLEntries, 0, &EH_##name));
      #include "hooklist.h"
      #undef HOOK_DEFINE

      EH_FORCE(LhSetGlobalExclusiveACL(ACLEntries, 0));
   }
   else if (dwHookApi == USE_DETOURS) {
      TRACE(_T("Install Detours Hooks\n"));
      DetourRestoreAfterWith();
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());

      /* attach detours hooks */
      #define HOOK_DEFINE(rettype, name, argtype) \
         TRACE(_T("Installing Hook %ls\n"), _T(#name)); \
         DetourAttach(&(PVOID&)ORIG_##name, IMPL_##name);
      #include "hooklist.h"
      #undef HOOK_DEFINE

      DetourTransactionCommit();
   }
   else {
      TRACE(_T("No Hooks Installed\n"));
   }
}

void HookUninstall(DWORD dwHookApi)
{
   if (dwHookApi == USE_EASYHOOK) {
      TRACE(_T("Uninstall Easyhook hooks\n"));
      EH_FORCE(LhUninstallAllHooks());
      EH_FORCE(LhWaitForPendingRemovals());
   }
   else if (dwHookApi == USE_DETOURS) {
      TRACE(_T("Uninstall Detours Hooks\n"));
      DetourTransactionBegin();
      DetourUpdateThread(GetCurrentThread());

      /* detach detours hooks */
      #define HOOK_DEFINE(rettype, name, argtype) \
         DetourDetach(&(PVOID&)ORIG_##name, IMPL_##name);
      #include "hooklist.h"
      #undef HOOK_DEFINE

      DetourTransactionCommit();
   }
   else {
      TRACE(_T("No Hooks Uninstalled\n"));
   }
}

enum { HOOK_ADD_THREAD, HOOK_REMOVE_THREAD };

void HookAddRemoveThread(DWORD dwHookApi, DWORD dwThreadId, int op_type)
{
}

#if 0
void HookAddRemoveThread(DWORD dwHookApi, DWORD dwThreadId, int op_type)
{
   static std::set<DWORD> thread_set;
   if (op_type == HOOK_ADD_THREAD)
      thread_set.insert(dwThreadId);
   else
      thread_set.erase(dwThreadId);

   LPCTSTR lpczOpName = op_type ? _T("Disable") : _T("Enable");

   if (dwHookApi == USE_EASYHOOK) {
      TRACE(_T("%s Easyhook Hooks in This Thread\n"), lpczOpName);

      DWORD dwCount = thread_set.size();
      DWORD *ACLEntries = (DWORD *)_alloca(sizeof(DWORD) * dwCount);
      std::copy(thread_set.begin(), thread_set.end(), ACLEntries);

      BOOL enabled;
      #define HOOK_DEFINE(rettype, name, argtype) \
         EH_FORCE(LhSetInclusiveACL(ACLEntries, dwCount, &EH_##name)); \
         EH_FORCE(LhIsThreadIntercepted(&EH_##name, 0, &enabled)); \
         TRACE(_T("Hook %ls enable: %d\n"), _T(#name), enabled);
      #include "hooklist.h"
      #undef HOOK_DEFINE

      EH_FORCE(LhSetGlobalInclusiveACL(ACLEntries, dwCount));
   }
   else if (dwHookApi == USE_DETOURS) {
      TRACE(_T("%s Detours Hooks in This Thread\n"), lpczOpName);
   }
   else {
      TRACE(_T("No Hooks Enabled/Disabled in This Thread\n"));
   }
}
#endif

void DumpMemory(void *p)
{
   return;
   TRACE(_T("DumpMemory at: %p\n"), p);
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 32; ++j) {
         int c = *((unsigned char *)p + i * 32 + j);
         TRACE(_T("%02x "), c);
      }
      TRACE(_T("\n"));
   }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
   TRACE(_T("Enter DllMain %ld\n"), reason);
   DWORD dwHookApi = HookApiRegOption();

   try {
      switch(reason) {
      case DLL_PROCESS_ATTACH:
         DumpMemory(ScriptItemize);
         DumpMemory(CreateFontIndirect);
         DumpMemory(CreateProcess);

         HookInstall(dwHookApi);
         /* go through to thread attach */
      case DLL_THREAD_ATTACH:
         HookAddRemoveThread(dwHookApi, GetCurrentThreadId(), HOOK_ADD_THREAD);
         break;
      case DLL_THREAD_DETACH:
         HookAddRemoveThread(dwHookApi, GetCurrentThreadId(), HOOK_REMOVE_THREAD);
         break;
      case DLL_PROCESS_DETACH:
         HookUninstall(dwHookApi);
      }
   }
   catch (EasyHookError &e) {
      TRACE(_T("EasyHook Error 0x%x: \"%ls\" (code: %d 0x%x)\n"), e.dwRetCode, e.lpczError, e.dwError);
   }

#if 0
   TRACE(_T("Calling ScriptItemize...\n"));
   ScriptItemize(0,0,0,0,0,0,0);
   DumpMemory(ScriptItemize);
   TRACE(_T("Calling CreateFontIndirect...\n"));
   CreateFontIndirect(0);
   DumpMemory(CreateFontIndirect);
   TRACE(_T("Calling CreateProcess...\n"));
   CreateProcess(0,0,0,0,0,0,0,0,0,0);
   DumpMemory(CreateProcess);
#endif
   return TRUE;
}
