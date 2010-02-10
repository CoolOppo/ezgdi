// API hook
//
// GetProcAddress�œ���call��i�֐��{�́j�𒼐ڏ��������A
// �����̃t�b�N�֐���jmp������B
//
// �����Ō���API���g�����́A�R�[�h����x�߂��Ă���call�B
// ������jmp�R�[�h�ɖ߂��B
//
// �}���`�X���b�h�� ������������call�����ƍ���̂ŁA
// CriticalSection�Ŕr�����䂵�Ă����B
//

#include "override.h"
#include "ft.h"
#include "fteng.h"
#include <locale.h>

#ifdef USE_DETOURS
#include <detours.h>

// DATA_foo�AORIG_foo �̂Q���܂Ƃ߂Ē�`����}�N��
#define HOOK_DEFINE(rettype, name, argtype) \
	rettype (WINAPI * ORIG_##name) argtype;
#include "hooklist.h"
#undef HOOK_DEFINE

//

#define HOOK_DEFINE(rettype, name, argtype) \
	ORIG_##name = name;
#pragma optimize("s", on)
static void hook_initinternal()
{
#include "hooklist.h"
}
#pragma optimize("", on)
#undef HOOK_DEFINE

#define HOOK_DEFINE(rettype, name, argtype) \
	DetourAttach(&(PVOID&)ORIG_##name, IMPL_##name);
static void hook_init()
{
	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

#include "hooklist.h"

	LONG error = DetourTransactionCommit();

	if (error != NOERROR) {
		TRACE(_T("hook_init error: %#x\n"), error);
	}
}
#undef HOOK_DEFINE
//

#define HOOK_DEFINE(rettype, name, argtype) \
	DetourDetach(&(PVOID&)ORIG_##name, IMPL_##name);
static void hook_term()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

#include "hooklist.h"

	LONG error = DetourTransactionCommit();

	if (error != NOERROR) {
		TRACE(_T("hook_term error: %#x\n"), error);
	}
}
#undef HOOK_DEFINE

#else /* EASYHOOK */

#include <easyhook.h>

#define HOOK_DEFINE(rettype, name, argtype, args, modname) \
	HOOK_TRACE_INFO HOOK_##name; \
	rettype (WINAPI * FUNC_##name) argtype = NULL; \
	rettype (WINAPI * ORIG_##name) argtype = NULL; \
	rettype WINAPI OLD_##name argtype { \
      ULONG ACLEntries[] = {0}; \
      LhSetExclusiveACL(ACLEntries, 1, &HOOK_##name); \
      rettype retval = ##name args; \
      LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name); \
      return retval; \
   }
#include "hooklist2.h"
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

#define HOOK_DEFINE(rettype, name, argtype, args, modname) \
	FUNC_##name = (rettype (WINAPI *) argtype) GetProcAddressInDll(_T(modname), #name); \
   ORIG_##name = OLD_##name;
static void hook_initinternal()
{
#include "hooklist2.h"
}
#undef HOOK_DEFINE

#define FORCE(expr) {if(!SUCCEEDED(NtStatus = (expr))) goto ERROR_ABORT;}

#define HOOK_DEFINE(rettype, name, argtype) \
    FORCE(LhInstallHook(FUNC_##name, IMPL_##name, (PVOID)0, &HOOK_##name));\
    FORCE(LhSetExclusiveACL(ACLEntries, 0, &HOOK_##name));

static void hook_init()
{
   ULONG ACLEntries[1] = {0};
	NTSTATUS NtStatus;
#include "hooklist.h"
	FORCE(LhSetGlobalExclusiveACL(ACLEntries, 0));
	return;

ERROR_ABORT:
	TRACE(_T("hook_init error: %#x\n"), NtStatus);
}
#undef HOOK_DEFINE
//

static void hook_term()
{
	LhUninstallAllHooks();
	LhWaitForPendingRemovals();
}

#endif

//---

CTlsData<CThreadLocalInfo>	g_TLInfo;
HINSTANCE					g_hinstDLL;
LONG						g_bHookEnabled;
#ifdef _DEBUG
HANDLE						g_hfDbgText;
#endif

//void InstallManagerHook();
//void RemoveManagerHook();

//#include "APITracer.hpp"

//�x�[�X�A�h���X��ς����������[�h�������Ȃ�
#if _DLL
#pragma comment(linker, "/base:0x06540000")
#endif
BOOL WINAPI  DllMain(HINSTANCE instance, DWORD reason, LPVOID /*lp*/)
{
	switch(reason) {
	case DLL_PROCESS_ATTACH:
		//����������
		//DLL_PROCESS_DETACH�ł͂���̋t���ɂ���
		//1. CRT�֐��̏�����
		//2. �N���e�B�J���Z�N�V�����̏�����
		//3. TLS�̏���
		//4. CGdippSettings�̃C���X�^���X�����AINI�ǂݍ���
		//5. ExcludeModule�`�F�b�N
		// 6. FreeType���C�u�����̏�����
		// 7. FreeTypeFontEngine�̃C���X�^���X����
		// 8. API���t�b�N
		// 9. Manager��GetProcAddress���t�b�N

		//1
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
		//_CrtSetBreakAlloc(100);

		//Opera��~�܂�`
		//Assert(GetModuleHandleA("opera.exe") == NULL);

		setlocale(LC_ALL, "");
		g_hinstDLL = instance;
		hook_initinternal();

//APITracer::Start(instance, APITracer::OutputFile);

		//2, 3
		CCriticalSectionLock::Init();
		if (!g_TLInfo.ProcessInit()) {
			return FALSE;
		}

		//4
		{
			CGdippSettings* pSettings = CGdippSettings::CreateInstance();
			if (!pSettings || !pSettings->LoadSettings(instance)) {
				CGdippSettings::DestroyInstance();
				return FALSE;
			}
		}
//		if (IsProcessExcluded()) {
//			return FALSE;
//		}

		//5
		if (!IsProcessExcluded()) {
			//6 �` 9
			// FreeType
			if (!FontLInit()) {
				return FALSE;
			}
			g_pFTEngine = new FreeTypeFontEngine;
			if (!g_pFTEngine) {
				return FALSE;
			}

			InterlockedExchange(&g_bHookEnabled, TRUE);
			hook_init();
//			InstallManagerHook();
		}

//APITracer::Finish();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		g_TLInfo.ThreadTerm();
		break;
	case DLL_PROCESS_DETACH:
//		RemoveManagerHook();
		if (InterlockedExchange(&g_bHookEnabled, FALSE)) {
			hook_term();
		}

		if (g_pFTEngine) {
			delete g_pFTEngine;
		}
		FontLFree();

		CGdippSettings::DestroyInstance();
		g_TLInfo.ProcessTerm();
		CCriticalSectionLock::Term();
		break;
	}
	return TRUE;
}
//EOF
