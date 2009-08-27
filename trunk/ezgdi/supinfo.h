#pragma once

#include <mmsystem.h>	//mmioFOURCC
#define FOURCC_GDIPP	mmioFOURCC('G', 'D', 'I', 'P')

typedef struct {
	int dummy;
	FOURCC magic;
//	BYTE reserved[256];
} GDIPP_CREATE_MAGIC;

//参照
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

EXTERN_C BOOL WINAPI GdippInjectDLL(const PROCESS_INFORMATION* ppi);
EXTERN_C LPWSTR WINAPI GdippEnvironment(DWORD& dwCreationFlags, LPVOID lpEnvironment);


//子プロセスにも自動でgdi++適用
template <typename _TCHAR, typename _STARTUPINFO, class _Function>
BOOL _CreateProcessAorW(const _TCHAR* lpApp, _TCHAR* lpCmd, LPSECURITY_ATTRIBUTES pa, LPSECURITY_ATTRIBUTES ta, BOOL bInherit, DWORD dwFlags, LPVOID lpEnv, const _TCHAR* lpDir, _STARTUPINFO* psi, LPPROCESS_INFORMATION ppi, _Function fn)
{
	return fn(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi);
#if 0
#ifdef _GDIPP_RUN_CPP
	const bool hookCP = true;
	const bool runGdi = true;
#else
	const CGdippSettings* pSettings = CGdippSettings::GetInstance();
	const bool hookCP = pSettings->HookChildProcesses();
	const bool runGdi = pSettings->RunFromGdiExe();
#endif

	if (!hookCP || (!lpApp && !lpCmd) || (dwFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) || !psi || psi->cb < sizeof(_STARTUPINFO)) {
		return fn(lpApp, lpCmd, pa, ta, bInherit, dwFlags, lpEnv, lpDir, psi, ppi);
	}
	PROCESS_INFORMATION _pi = { 0 };
	if (!ppi) {
		ppi = &_pi;
	}

	_STARTUPINFO& si = *(_STARTUPINFO*)_alloca(psi->cb);
	memcpy(&si, psi, psi->cb);
	psi = &si;

	GDIPP_CREATE_MAGIC gppcm;
	if (runGdi && !si.cbReserved2) {
		FillGdiPPStartupInfo(si, gppcm);
	}

	LPWSTR pEnvW = GdippEnvironment(dwFlags, lpEnv);
	if (pEnvW) {
		lpEnv = pEnvW;
	}

	if (!fn(lpApp, lpCmd, pa, ta, bInherit, dwFlags | CREATE_SUSPENDED, lpEnv, lpDir, &si, ppi)) {
		ZeroMemory(ppi, sizeof(*ppi));
		free(pEnvW);
		return FALSE;
	}

	GdippInjectDLL(ppi);
	if (!(dwFlags & CREATE_SUSPENDED)) {
		ResumeThread(ppi->hThread);
	}
	free(pEnvW);
	return TRUE;
#endif
}
