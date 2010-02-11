#include "EasyHook.h"
#include <stdio.h>
#include <conio.h>

BOOL WINAPI MessageBeepHook(__in UINT uType)
{
    printf("New Silent MessageBeep\n");
    return TRUE;
}

BOOL WINAPI IMPL_GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize)
{
    printf("GetTextExtentPoint32W\n");
    return TRUE;
}

BOOL WINAPI IMPL_GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize)
{
    printf("GetTextExtentPoint32A\n");
    return TRUE;
}

HFONT WINAPI IMPL_CreateFontIndirectW(CONST LOGFONTW *lplf)
{
    printf("CreateFontIndirectW\n");
    return NULL;
}

HFONT (WINAPI * ORIG_CreateFontIndirectW)(CONST LOGFONTW *lplf);

BOOL (WINAPI * ORIG_MessageBeepHook)(__in UINT);
BOOL (WINAPI * ORIG_GetTextExtentPoint32W)(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize);
BOOL (WINAPI * ORIG_GetTextExtentPoint32A)(HDC hdc, LPCSTR lpString, int cbString, LPSIZE lpSize);

#define FORCE(expr)     {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}

extern "C" int main(int argc, wchar_t* argv[])
{
    TRACED_HOOK_HANDLE      hHook = new HOOK_TRACE_INFO();
    NTSTATUS                NtStatus;
    ULONG                   ACLEntries[1] = {0};
    UNICODE_STRING*         NameBuffer = NULL;

    ORIG_CreateFontIndirectW = CreateFontIndirectW;

    FORCE(LhInstallHook(
            ORIG_CreateFontIndirectW,
            IMPL_CreateFontIndirectW,
            (PVOID)0,
            hHook));
    FORCE(LhSetInclusiveACL(ACLEntries, 1, hHook));

    CreateFontIndirectW(0);
    CreateFontW(10, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"system");
    LOGFONTA lf = {};
    CreateFontIndirectA(&lf);
    CreateFontA(12, 0, 0, 0, 400, 0, 0, 0, 2, 0, 0, 0, 0, "MARLETT");

#if 0
    ORIG_GetTextExtentPoint32A = GetTextExtentPoint32A;
    FORCE(LhInstallHook(
            ORIG_GetTextExtentPoint32A,
            IMPL_GetTextExtentPoint32A,
            (PVOID)0,
            hHook));
    HDC hdc = GetDC(NULL);
    SIZE size;
    FORCE(LhSetInclusiveACL(ACLEntries, 1, hHook));
    GetTextExtentPoint32W(hdc, L"abc", 3, &size);
    GetTextExtentPointW(hdc, L"abc", 3, &size);
    GetTextExtentPoint32A(hdc, "abc", 3, &size);
    GetTextExtentPointA(hdc, "abc", 3, &size);
#endif

#if 0
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
#endif
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

