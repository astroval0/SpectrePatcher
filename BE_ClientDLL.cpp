#include <windows.h>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>

void Log(const std::string& message) {
    if (std::ofstream logfile("be_proxy.log", std::ios_base::app); logfile.is_open()) {
        const auto now = std::chrono::system_clock::now();
        const auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm timeinfo{};
        localtime_s(&timeinfo, &in_time_t);
        logfile << std::put_time(&timeinfo, "%Y-%m-%d %X") << " - " << message << std::endl;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, const DWORD  ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Log("BE_ClientDLL.dll loaded");
    }
    return TRUE;
}

extern "C" {
    __declspec(dllexport) bool __fastcall Init(__int64 a1, void* a2, void* a3) { return true; }
    __declspec(dllexport) void __stdcall Run() {}
    __declspec(dllexport) void __stdcall Shutdown() {}
}