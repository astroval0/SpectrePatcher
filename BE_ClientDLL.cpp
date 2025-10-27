#include <windows.h>
#include <chrono>
#include <iostream>

uintptr_t BaseAddress = 0;
static std::atomic g_Started{ false };

constexpr uintptr_t RVA_PragmaEngineString = 0x7680E90;

void* g_PatchedAddress = nullptr;
DWORD g_PageSize = 0;

LONG WINAPI MemoryGuardHandler(PEXCEPTION_POINTERS ExceptionInfo) {
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        if ((void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1] >= g_PatchedAddress &&
            (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1] < (char*)g_PatchedAddress + g_PageSize)
        {
            DWORD oldProtect;
            VirtualProtect(g_PatchedAddress, g_PageSize, PAGE_READONLY, &oldProtect);
            ExceptionInfo->ContextRecord->EFlags |= 0x100;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    else if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP) {
        if ((void*)ExceptionInfo->ExceptionRecord->ExceptionAddress >= g_PatchedAddress &&
            (void*)ExceptionInfo->ExceptionRecord->ExceptionAddress < (char*)g_PatchedAddress + g_PageSize)
        {
            DWORD oldProtect;
            VirtualProtect(g_PatchedAddress, g_PageSize, PAGE_NOACCESS, &oldProtect);
            ExceptionInfo->ContextRecord->EFlags &= ~0x100;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL __stdcall DecryptPage(void* pageAddr) {
	CONTEXT ctx{};
	ctx.ContextFlags = CONTEXT_FULL;
	ctx.Rip = reinterpret_cast<DWORD64>(pageAddr);
	ctx.Rsp = (reinterpret_cast<DWORD64>(&ctx) - 0x200) & -0x10;
	ctx.Rbp = ctx.Rsp;

	DWORD64 imageBase = 0;
	DWORD64 establisher = 0;
	PVOID handler = nullptr;

	__try {
		auto fn = RtlLookupFunctionEntry(ctx.Rip, &imageBase, nullptr);
		RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip,
						 fn, &ctx, &handler, &establisher, nullptr);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		std::cout << "poking theia" << std::endl;
	}
	return TRUE;
}

void TheFinalPatch() {
    uintptr_t targetAddress = BaseAddress + RVA_PragmaEngineString;
    std::cout << "targeting string at: 0x" << std::hex << targetAddress << std::dec << std::endl;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    g_PageSize = sysInfo.dwPageSize;
    g_PatchedAddress = (void*)(targetAddress & ~(g_PageSize - 1));

    std::cout << "aligning to page start: 0x" << std::hex << g_PatchedAddress << std::dec << std::endl;

    DecryptPage((void*)targetAddress);

    const wchar_t* newHost = L"astro-dev.uk";
    size_t originalLenInBytes = wcslen(L"pragmaengine.com") * sizeof(wchar_t);
    size_t newHostLenInBytes = wcslen(newHost) * sizeof(wchar_t);

    if (newHostLenInBytes > originalLenInBytes) {
        std::cout << "new string too long :<" << std::endl;
        return;
    }

    DWORD oldProtect;
    if (VirtualProtect((void*)targetAddress, originalLenInBytes + 2, PAGE_READWRITE, &oldProtect)) {
        memcpy((void*)targetAddress, newHost, newHostLenInBytes);
        memset((void*)(targetAddress + newHostLenInBytes), 0, originalLenInBytes - newHostLenInBytes + 2);

        std::wcout << L"whitelist overwritten with '" << newHost << L"'." << std::endl;
    } else {
        std::cout << "VP failed error: " << GetLastError() << std::endl;
        return;
    }

    if (VirtualProtect(g_PatchedAddress, g_PageSize, PAGE_NOACCESS, &oldProtect)) {
        std::cout << "mem trap set" << std::endl;
    } else {
        std::cout << "couldnt set mem trap error: " << GetLastError() << std::endl;
    }
}

static void InitConsole() {
	AllocConsole();
	FILE* f = nullptr;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	std::cout.sync_with_stdio(false);
}

static DWORD WINAPI MainThreadProc(LPVOID) {
	InitConsole();
    std::cout << "setting up mem trap" << std::endl;
    bool expected = false;
    if (!g_Started.compare_exchange_strong(expected, true)) return 0;

    AddVectoredExceptionHandler(1, MemoryGuardHandler);
    std::cout << "VEH installed." << std::endl;

    BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if (!BaseAddress) {
        std::cout << "GetModuleHandle failed." << std::endl;
        return 0;
    }

    Sleep(2000);

    TheFinalPatch();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        if (HANDLE h = CreateThread(nullptr, 0, MainThreadProc, nullptr, 0, nullptr)) {
            CloseHandle(h);
        }
    }
    return TRUE;
}

extern "C" {
    __declspec(dllexport) bool __fastcall Init(__int64 a1, void* a2, void* a3) { return true; }
    __declspec(dllexport) void __stdcall Run() {}
    __declspec(dllexport) void __stdcall Shutdown() {}
}