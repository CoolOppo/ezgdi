#ifndef _GDIPP_EXE
#include "settings.h"
#include <tlhelp32.h>
#include <shlwapi.h>	//DLLVERSIONINFO

// win2k以降
//#pragma comment(linker, "/subsystem:windows,5.0")

EXTERN_C LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
	//何もしない
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
	pdvi->dwMajorVersion	= HIWORD(pvffi->dwFileVersionMS);
	pdvi->dwMinorVersion	= LOWORD(pvffi->dwFileVersionMS) * 10 + HIWORD(pvffi->dwFileVersionLS);
	pdvi->dwBuildNumber		= LOWORD(pvffi->dwFileVersionLS);
	pdvi->dwPlatformID		= DLLVER_PLATFORM_NT;

	if (pdvi->cbSize < sizeof(DLLVERSIONINFO2)) {
		return S_OK;
	}

	DLLVERSIONINFO2* pdvi2 = (DLLVERSIONINFO2*)pdvi;
	pdvi2->ullVersion		= MAKEDLLVERULL(pdvi->dwMajorVersion, pdvi->dwMinorVersion, pdvi->dwBuildNumber, 2);
	return S_OK;
}

#endif	//!_GDIPP_EXE

#if 0

#ifndef Assert
#include <crtdbg.h>
#define Assert	_ASSERTE
#endif	//!Assert

#include <strsafe.h>

//kernel32専用GetProcAddressモドキ
FARPROC K32GetProcAddress(LPCSTR lpProcName)
{
#ifndef _WIN64
	//序数渡しには対応しない
	Assert(!IS_INTRESOURCE(lpProcName));

	//kernel32のベースアドレス取得
	LPBYTE pBase = (LPBYTE)GetModuleHandleA("kernel32.dll");

	//この辺は100%成功するはずなのでエラーチェックしない
	PIMAGE_DOS_HEADER pdosh = (PIMAGE_DOS_HEADER)pBase;
	Assert(pdosh->e_magic == IMAGE_DOS_SIGNATURE);
	PIMAGE_NT_HEADERS pnth = (PIMAGE_NT_HEADERS)(pBase + pdosh->e_lfanew);
	Assert(pnth->Signature == IMAGE_NT_SIGNATURE);

	const DWORD offs = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	const DWORD size = pnth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	if (offs == 0 || size == 0) {
		return NULL;
	}

	PIMAGE_EXPORT_DIRECTORY pdir = (PIMAGE_EXPORT_DIRECTORY)(pBase + offs);
	DWORD*	pFunc = (DWORD*)(pBase + pdir->AddressOfFunctions);
	WORD*	pOrd  = (WORD*)(pBase + pdir->AddressOfNameOrdinals);
	DWORD*	pName = (DWORD*)(pBase + pdir->AddressOfNames);

	for(DWORD i=0; i<pdir->NumberOfFunctions; i++) {
		for(DWORD j=0; j<pdir->NumberOfNames; j++) {
			if(pOrd[j] != i)
				continue;

			if(strcmp((LPCSTR)pBase + pName[j], lpProcName) != 0)
				continue;

			return (FARPROC)(pBase + pFunc[i]);
		}
	}
	return NULL;
#endif
}


#include <pshpack1.h>
class opcode_data {
private:
	BYTE	code[0x100];

	//注: dllpathをWORD境界にしないと場合によっては正常に動作しない
	WCHAR	dllpath[MAX_PATH];

public:
	opcode_data()
	{
		//int 03hで埋める
		FillMemory(this, sizeof(*this), 0xcc);
	}
	bool init(LPVOID remoteaddr, LONG orgEIP)
	{
		//WORD境界チェック
		C_ASSERT((offsetof(opcode_data, dllpath) & 1) == 0);

		register BYTE* p = code;

#define emit_(t,x)	*(t* UNALIGNED)p = (t)(x); p += sizeof(t)
#define emit_db(b)	emit_(BYTE, b)
#define emit_dw(w)	emit_(WORD, w)
#define emit_dd(d)	emit_(DWORD, d)

		//なぜかGetProcAddressでLoadLibraryWのアドレスが正しく取れないことがあるので
		//kernel32のヘッダから自前で取得する
		FARPROC pfn = K32GetProcAddress("LoadLibraryW");
		if(!pfn)
			return false;

		emit_db(0x60);		//pushad

#if _DEBUG
emit_dw(0xC033);	//xor eax, eax
emit_db(0x50);		//push eax
emit_db(0x50);		//push eax
emit_db(0x68);		//push dllpath
emit_dd((LONG)remoteaddr + offsetof(opcode_data, dllpath));
emit_db(0x50);		//push eax
emit_db(0xB8);		//mov eax, MessageBoxW
emit_dd((LONG)MessageBoxW);
emit_dw(0xD0FF);	//call eax
#endif

		emit_db(0x68);		//push dllpath
		emit_dd((LONG)remoteaddr + offsetof(opcode_data, dllpath));
		emit_db(0xB8);		//mov eax, LoadLibraryW
		emit_dd(pfn);
		emit_dw(0xD0FF);	//call eax

		emit_db(0x61);		//popad
		emit_db(0xE9);		//jmp original_EIP
		emit_dd(orgEIP - (LONG)remoteaddr - (p - code) - sizeof(LONG));

		// gdi++.dllのパス
		return !!GetModuleFileNameW(GetDLLInstance(), dllpath, MAX_PATH);
	}
};
#include <poppack.h>

// 止めているプロセスにLoadLibraryするコードを注入
EXTERN_C BOOL WINAPI GdippInjectDLL(const PROCESS_INFORMATION* ppi)
{
	CONTEXT ctx = { 0 };
	ctx.ContextFlags = CONTEXT_CONTROL;
	//CREATE_SUSPENDEDなので基本的に成功するはず
	if(!GetThreadContext(ppi->hThread, &ctx))
		return false;

	opcode_data local;
	opcode_data* remote = (opcode_data*)VirtualAllocEx(ppi->hProcess, NULL, sizeof(opcode_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!remote)
		return false;

	if(!local.init(remote, ctx.Eip)
		|| !WriteProcessMemory(ppi->hProcess, remote, &local, sizeof(opcode_data), NULL)) {
			VirtualFreeEx(ppi->hProcess, remote, 0, MEM_RELEASE);
			return false;
	}

	FlushInstructionCache(ppi->hProcess, remote, sizeof(opcode_data));
	ctx.Eip = (DWORD)remote;
	return !!SetThreadContext(ppi->hThread, &ctx);
}

template <typename _TCHAR>
int strlendb(const _TCHAR* psz)
{
	const _TCHAR* p = psz;
	while (*p) {
		for (; *p; p++);
		p++;
	}
	return p - psz + 1;
}

template <typename _TCHAR>
_TCHAR* strdupdb(const _TCHAR* psz, int pad)
{
	int len = strlendb(psz);
	_TCHAR* p = (_TCHAR*)calloc(sizeof(_TCHAR), len + pad);
	if(p) {
		memcpy(p, psz, sizeof(_TCHAR) * len);
	}
	return p;
}

#include "array.h"

bool MultiSzToArray(LPWSTR p, CArray<LPWSTR>& arr)
{
	for (; *p; ) {
		LPWSTR cp = _wcsdup(p);
		if(!cp || !arr.Add(cp)) {
			free(cp);
			return false;
		}
		for (; *p; p++);
		p++;
	}
	return true;
}

LPWSTR ArrayToMultiSz(CArray<LPWSTR>& arr)
{
	size_t cch = 1;
	for (int i=0; i<arr.GetSize(); i++) {
		cch += wcslen(arr[i]) + 1;
	}

	LPWSTR pmsz = (LPWSTR)calloc(sizeof(WCHAR), cch);
	if (!pmsz)
		return NULL;

	LPWSTR p = pmsz;
	for (int i=0; i<arr.GetSize(); i++) {
		StringCchCopyExW(p, cch, arr[i], &p, &cch, STRSAFE_NO_TRUNCATION);
		p++;
	}
	*p = 0;
	return pmsz;
}

bool AddPathEnv(CArray<LPWSTR>& arr, LPWSTR dir, int dirlen)
{
	for (int i=0; i<arr.GetSize(); i++) {
		LPWSTR env = arr[i];
		if (_wcsnicmp(env, L"PATH=", 5)) {
			continue;
		}

		LPWSTR p = env + 5;
		LPWSTR pp = p;
		for (; ;) {
			for (; *p && *p != L';'; p++);
			int len = p - pp;
			if (len == dirlen && !_wcsnicmp(pp, dir, dirlen)) {
				return false;
			}
			if (!*p)
				break;
			pp = p + 1;
			p++;
		}

		size_t cch = wcslen(env) + MAX_PATH + 4;
		env = (LPWSTR)realloc(env, sizeof(WCHAR) * cch);
		if(env) {
			StringCchCatW(env, cch, L";");
			StringCchCatW(env, cch, dir);
			arr[i] = env;
			return true;
		}
		return false;
	}

	size_t cch = dirlen + sizeof("PATH=") + 1;
	LPWSTR p = (LPWSTR)calloc(sizeof(WCHAR), cch);
	if(p) {
		StringCchCopyW(p, cch, L"PATH=");
		StringCchCatW(p, cch, dir);
		if (arr.Add(p)) {
			return true;
		}
		free(p);
	}
	return false;
}

EXTERN_C LPWSTR WINAPI GdippEnvironment(DWORD& dwCreationFlags, LPVOID lpEnvironment)
{
	WCHAR dir[MAX_PATH];
	GetModuleFileNameW(GetDLLInstance(), dir, MAX_PATH);
	PathRemoveFileSpec(dir);
	int dirlen = wcslen(dir);

	LPWSTR pEnvW = NULL;
	if (lpEnvironment) {
		if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT) {
			pEnvW = strdupdb((LPCWSTR)lpEnvironment, MAX_PATH + 1);
		} else {
			int alen = strlendb((LPCSTR)lpEnvironment);
			int wlen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpEnvironment, alen, NULL, 0) + 1;
			pEnvW = (LPWSTR)calloc(sizeof(WCHAR), wlen + MAX_PATH + 1);
			if (pEnvW) {
				MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpEnvironment, alen, pEnvW, wlen);
			}
		}
	} else {
		LPWSTR block = (LPWSTR)GetEnvironmentStringsW();
		if (block) {
			pEnvW = strdupdb(block, MAX_PATH + 1);
			FreeEnvironmentStrings(block);
		}
	}

	if (!pEnvW) {
		return NULL;
	}

	CArray<LPWSTR> envs;
	bool ret = MultiSzToArray(pEnvW, envs);
	free(pEnvW);
	pEnvW = NULL;
	
	if (ret) {
		ret = AddPathEnv(envs, dir, dirlen);
	}
	if (ret) {
		pEnvW = ArrayToMultiSz(envs);
	}

	for (int i=0; i<envs.GetSize(); free(envs[i++]));

	if (!pEnvW) {
		return NULL;
	}

#ifdef _DEBUG
	{
		LPWSTR tmp = strdupdb(pEnvW, 0);
		LPWSTR tmpe = tmp + strlendb(tmp);
		PathRemoveFileSpec(dir);
		for (LPWSTR z=tmp; z<tmpe; z++)if(!*z)*z=L'\n';
			StringCchCatW(dir,MAX_PATH,L"\\");
			StringCchCatW(dir,MAX_PATH,L"gdienv.txt");
			HANDLE hf = CreateFileW(dir,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
			if(hf) {
			DWORD cb;
			WORD w = 0xfeff;
			WriteFile(hf,&w, sizeof(WORD), &cb, 0);
			WriteFile(hf,tmp, sizeof(WCHAR) * (tmpe - tmp), &cb, 0);
			SetEndOfFile(hf);
			CloseHandle(hf);
			free(tmp);
		}
	}
#endif

	dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	return pEnvW;
}
#endif
