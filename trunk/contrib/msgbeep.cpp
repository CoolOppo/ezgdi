#include <easyhook.h>
#include <stdio.h>

HOOK_TRACE_INFO hinfo = {NULL};
ULONG acl_entries[128] = {};
int thread_num = 0;

BOOL WINAPI MessageBeep_hook(UINT type)
{
   printf("Beep!\n");
   return TRUE;
}

void hook_install()
{
   LhInstallHook(MessageBeep, MessageBeep_hook, 0, &hinfo);
}

void hook_uninstall()
{
   LhUninstallAllHooks();
   LhWaitForPendingRemovals();
}

void hook_enable_thread()
{
   LhSetInclusiveACL(acl_entries, ++thread_num, &hinfo);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
   switch(reason) {
   case DLL_PROCESS_ATTACH:
      hook_install();
      /* go through to enable current thread */
   case DLL_THREAD_ATTACH:
      hook_enable_thread();
      break;
   case DLL_THREAD_DETACH:
      break;
   case DLL_PROCESS_DETACH:
      hook_uninstall();
   }
   return TRUE;
}
