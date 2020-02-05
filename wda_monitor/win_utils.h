#pragma once

#define XFATAL_CODE(__code) xEpicFail(__FILE__, __LINE__, (__code))
#define XFATAL() XFATAL_CODE(0)

#define XGUARD_CODE(__cond, __code) xGuard((__cond), __FILE__, __LINE__, (__code))
#define XGUARD_WIN(__cond) XGUARD_CODE((__cond), GetLastError())
#define XGUARD(__cond) XGUARD_CODE((__cond), 0)

#define XGUARD_HR(__code) xGuard((__code), __FILE__, __LINE__)

inline void xEpicFail(const char* file, int line, int code)
{
	printf("EPIC FAIL -> FILE:%s LINE:%d CODE=(int)%d\n", file, line, (int)code); 
	throw std::runtime_error("EPIC FAIL");
}

inline void xGuard(bool cond, const char* file, int line, int code) { if (!cond) { xEpicFail(file, line, code); } }
inline void xGuard(HRESULT code, const char* file, int line) { if FAILED(code) { xEpicFail(file, line, code); } }

//=======================================================================================

inline bool xIsHandleValid(PVOID Handle) { return (Handle != nullptr && Handle != INVALID_HANDLE_VALUE); }

template<typename T, typename Traits>
class TScopedHandle
{
public:
	T Handle;

	inline TScopedHandle() : Handle(nullptr) {}
	inline explicit TScopedHandle(T h) : Handle(h) {}
	inline TScopedHandle(TScopedHandle& other) { Handle = other.Handle; other.Handle = nullptr; }

	inline ~TScopedHandle() { Close(); }
	inline void Close() { Traits::Close(Handle); }
	inline void Dismiss() { Handle = nullptr; }

	inline T& operator*() { return Handle; }
	inline T* operator&() { return &Handle; }

	inline const T& operator*() const { return Handle; }
	inline const T* operator&() const { return &Handle; }

	inline TScopedHandle& operator=(T h) { Close(); Handle = h; return *this; }
	inline TScopedHandle& operator=(TScopedHandle& other) { Close(); Handle = other.Handle; other.Handle = nullptr; return *this; }

	inline operator bool() const { return xIsHandleValid(Handle); }
};

struct HANDLE_traits {
	static void Close(HANDLE& Value) throw() { if (xIsHandleValid(Value)) { CloseHandle(Value); Value = nullptr; } }
};

typedef TScopedHandle<HANDLE, HANDLE_traits> XScopedHandle;

//=======================================================================================

VOID xAdjustPrivilege(LPCWSTR PrivilegeName, BOOL Enable)
{
	TOKEN_PRIVILEGES Privilege;
	ZeroMemory(&Privilege, sizeof(Privilege));
	Privilege.PrivilegeCount = 1;
	Privilege.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);

	XGUARD_WIN(LookupPrivilegeValueW(nullptr, PrivilegeName, &Privilege.Privileges[0].Luid));

	XScopedHandle Token;
	XGUARD_WIN(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token));

	XGUARD_WIN(AdjustTokenPrivileges(*Token, FALSE, &Privilege, sizeof(Privilege), nullptr, nullptr));
}

DWORD xGetPidByName(LPCWSTR name)
{
	DWORD Pid = 0;

	XScopedHandle Snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	XGUARD_WIN(Snapshot);

	PROCESSENTRY32W Entry;
	ZeroMemory(&Entry, sizeof(Entry));
	Entry.dwSize = sizeof(Entry);
	XGUARD_WIN(Process32FirstW(Snapshot.Handle, &Entry));

	do
	{
		if (_wcsicmp(Entry.szExeFile, name) == 0)
		{
			Pid = Entry.th32ProcessID;
			break;
		}
	}
	while (Process32NextW(Snapshot.Handle, &Entry));

	return Pid;
}

static inline bool xCompareBytes(const uint8_t &a, const uint8_t &b)
{
	return (a == b || b == 0xCC); // use 0xCC (int3) as wildcard
}

PVOID xFindPattern(HANDLE Process, LPVOID ImageBase, const std::vector<uint8_t>& Pattern)
{
	PVOID Result = nullptr;

	uint8_t* Ptr = (uint8_t*)ImageBase;

	MEMORY_BASIC_INFORMATION MemInfo;
	ZeroMemory(&MemInfo, sizeof(MemInfo));

	std::vector<uint8_t> Buffer;
	std::vector<uint8_t>::iterator Iter;

	for (;;)
	{
		auto QuerySize = VirtualQueryEx(Process, Ptr, &MemInfo, sizeof(MemInfo));
		if (QuerySize != sizeof(MemInfo))
		{
			DWORD Err = GetLastError();
			XGUARD_CODE(ERROR_INVALID_PARAMETER == Err, Err);
			break;
		}

		const bool bExecutable = (
			(MemInfo.Protect & PAGE_EXECUTE) != 0 || 
			(MemInfo.Protect & PAGE_EXECUTE_READ) != 0 || 
			(MemInfo.Protect & PAGE_EXECUTE_READWRITE) != 0 || 
			(MemInfo.Protect & PAGE_EXECUTE_WRITECOPY) != 0);

		if (bExecutable && ((MemInfo.Protect & PAGE_GUARD) == 0) && ((MemInfo.Protect & PAGE_NOACCESS) == 0))
		{
			if (Buffer.size() < MemInfo.RegionSize)
			{
				Buffer.resize(MemInfo.RegionSize);
			}

			SIZE_T NumBytes = 0;
			XGUARD_WIN(ReadProcessMemory(Process, MemInfo.BaseAddress, &Buffer[0], MemInfo.RegionSize, &NumBytes));

			auto BufferEnd = Buffer.begin() + MemInfo.RegionSize;
			Iter = Buffer.begin();

			if ((Iter = std::search(Iter, BufferEnd, Pattern.begin(), Pattern.end(), xCompareBytes)) != BufferEnd)
			{
				Result = (uint8_t*)MemInfo.BaseAddress + (Iter - Buffer.begin());
				break;
			}
		}

		Ptr += MemInfo.RegionSize;
	}

	return Result;
}

void xProtectWriteMemory(HANDLE Process, const std::vector<uint8_t>& OrigBytes, const std::vector<uint8_t>& PatchBytes, PVOID Addr, SIZE_T Offset)
{
	printf("xPatchMemory Addr=%p Offset=%I64u\n", Addr, (UINT64)Offset);

	DWORD Prot = 0;
	XGUARD_WIN(VirtualProtectEx(Process, Addr, OrigBytes.size(), PAGE_EXECUTE_READWRITE, &Prot));

	SIZE_T IoSize = 0;
	XGUARD_WIN(WriteProcessMemory(Process, (PVOID)((UINT64)Addr + (UINT64)Offset), &PatchBytes[0], PatchBytes.size(), &IoSize));

	DWORD Prot2 = 0;
	XGUARD_WIN(VirtualProtectEx(Process, Addr, OrigBytes.size(), Prot, &Prot2));

	printf("xPatchMemory DONE\n");
}

PVOID xPatchProcess(LPCWSTR ProcName, const std::vector<uint8_t>& OrigBytes, const std::vector<uint8_t>& PatchBytes, PVOID PatchAddr, SIZE_T PatchOffset)
{
	printf("xPatchProcess Name=\"%S\"\n", ProcName);

	XGUARD(OrigBytes.size());
	XGUARD(PatchBytes.size());

	DWORD ProcId = 0;
	XScopedHandle Process;

	if (ProcName)
	{
		ProcId = xGetPidByName(ProcName);
		XGUARD_WIN(ProcId);

		xAdjustPrivilege(L"SeDebugPrivilege", TRUE);

		Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcId);
		XGUARD_WIN(Process);
	}
	else
	{
		ProcId = GetCurrentProcessId();
		Process = GetCurrentProcess();
	}

	PVOID Result = nullptr;

	if (PatchAddr)
	{
		xProtectWriteMemory(*Process, OrigBytes, PatchBytes, PatchAddr, PatchOffset);
		Result = PatchAddr;
	}
	else
	{
		XScopedHandle Snap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcId));
		XGUARD_WIN(Snap);

		MODULEENTRY32 Entry;
		ZeroMemory(&Entry, sizeof(Entry));
		Entry.dwSize = sizeof(Entry);
		XGUARD_WIN(Module32First(*Snap, &Entry));

		do
		{
			if (Entry.th32ProcessID == ProcId)
			{
				PVOID Addr = xFindPattern(*Process, Entry.modBaseAddr, OrigBytes);
				if (Addr)
				{
					xProtectWriteMemory(*Process, OrigBytes, PatchBytes, Addr, PatchOffset);
					Result = Addr;
					break;
				}
			}
		} 
		while (Module32Next(*Snap, &Entry));
	}

	return Result;
}

//=======================================================================================

inline uint8_t xParseHex(uint8_t Val)
{
	if (Val >= '0' && Val <= '9') return (Val - '0');
	if (Val >= 'a' && Val <= 'f') return (Val - 'a') + 10;
	if (Val >= 'A' && Val <= 'F') return (Val - 'A') + 10;
	XFATAL();
	return 0;
}

inline uint8_t xParseByte(const char* Str)
{
	uint8_t hi = xParseHex((uint8_t)Str[0]);
	uint8_t lo = xParseHex((uint8_t)Str[1]);
	return ((hi << 4) | lo);
}

std::vector<uint8_t> xParseByteArray(LPCSTR Str)
{
	XGUARD(Str);

	const size_t Len = strlen(Str);
	XGUARD(Len);

	size_t SpaceCount = 0;
	for (size_t i = 0; i < Len; ++i)
	{
		if (Str[i] == ' ') SpaceCount++;
	}

	const size_t NumBytes = (SpaceCount + 1);
	XGUARD(Len == (NumBytes * 2 + SpaceCount));

	std::vector<uint8_t> Vec;
	Vec.resize(NumBytes);

	for (size_t i = 0; i < NumBytes; i++)
	{
		Vec[i] = xParseByte(Str);
		Str += 3;
	}

	return Vec;
}
