#include "EasyHook.h"
#include <stdio.h>
#include <conio.h>

BOOL WINAPI MessageBeepHook(__in UINT uType)
{
    printf("New Silent MessageBeep\n");
    return TRUE;
}

BOOL  (WINAPI * ORIG_MessageBeepHook)(__in UINT);

#define FORCE(expr)     {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}

extern "C" int main(int argc, wchar_t* argv[])
{
    TRACED_HOOK_HANDLE      hHook = new HOOK_TRACE_INFO();
    NTSTATUS                NtStatus;
    ULONG                   ACLEntries[1] = {0};
    UNICODE_STRING*         NameBuffer = NULL;
    HANDLE                                  hRemoteThread;

    ORIG_MessageBeepHook = MessageBeep;
    /*
        The following shows how to install and remove local hooks...
    */
    FORCE(LhInstallHook(
            ORIG_MessageBeepHook,
            MessageBeepHook,
            (PVOID)0,
            hHook));

    printf(".\n");
    // won't invoke the hook handler because hooks are inactive after installation
    MessageBeep(123);
    getch();

    BOOL flags = 1;
    FORCE(LhIsThreadIntercepted(hHook, 0, &flags));
    printf("Intercepted %d\n", flags);
    // activate the hook for the current thread
    FORCE(LhSetInclusiveACL(ACLEntries, 1, hHook));
    FORCE(LhIsThreadIntercepted(hHook, 0, &flags));
    printf("Intercepted %d\n", flags);

    printf(".\n");
    // will be redirected into the handler...
    MessageBeep(123);
    getch();

    FORCE(LhSetGlobalExclusiveACL(ACLEntries, 1));
    printf(".\n");
    // will be redirected into the handler...
    MessageBeep(123);
    getch();

    FORCE(LhSetGlobalInclusiveACL(ACLEntries, 1));
    printf(".\n");
    // will be redirected into the handler...
    MessageBeep(123);
    getch();

    printf(".\n");
    // won't invoke the hook handler because hooks are inactive after installation
    ORIG_MessageBeepHook(123);
    getch();

    // this will also invalidate "hHook", because it is a traced handle...
    LhUninstallAllHooks();
    // this will do nothing because the hook is already removed...
    LhUninstallHook(hHook);

    printf(".\n");
    // will be redirected into the handler...
    MessageBeep(123);
    getch();

    // now we can safely release the traced handle
    delete hHook;

    hHook = NULL;

    // even if the hook is removed, we need to wait for memory release
    LhWaitForPendingRemovals();

    return 0;

ERROR_ABORT:

    if(hHook != NULL)
        delete hHook;

    if(NameBuffer != NULL)
        free(NameBuffer );

    printf("\n[Error(0x%p)]: \"%S\" (code: %d {0x%p})\n", (PVOID)NtStatus, RtlGetLastErrorString(), RtlGetLastError(), (PVOID)RtlGetLastError());

    _getch();

    return NtStatus;
}

