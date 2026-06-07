#include "SEH.h"
#include <Windows.h>
#include <DbgHelp.h>
#include <fstream>
#include <format>
#include <string>
#include <chrono>
#include "API/LunarTear.h"
#include <filesystem>
#pragma comment(lib, "dbghelp.lib")

LONG WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS* exceptionInfo) {
    auto* record = exceptionInfo->ExceptionRecord;
    DWORD code = record->ExceptionCode;

    if (code == 0x406D1388 || // Thread naming
        code == EXCEPTION_BREAKPOINT ||
        code == 0x40010006) { // DBG_PRINTEXCEPTION_C
        return EXCEPTION_CONTINUE_SEARCH;
    }

    bool isCrash = (
        code == EXCEPTION_ACCESS_VIOLATION ||
        code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
        code == EXCEPTION_STACK_OVERFLOW ||
        code == EXCEPTION_PRIV_INSTRUCTION
        );

    if (!isCrash) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    try {
        std::error_code ec;
        std::filesystem::create_directories("LunarTear/crash", ec);

        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm;
        localtime_s(&now_tm, &now_c);

        auto timestamp = std::format("{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}",
            now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday,
            now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);

        std::ofstream logFile("LunarTear/crash/" + timestamp + ".txt", std::ios::trunc);
        if (!logFile.is_open()) return EXCEPTION_CONTINUE_SEARCH;

        auto* context = exceptionInfo->ContextRecord;

        auto GetModuleInfo = [](DWORD64 addr) -> std::pair<std::string, DWORD64> {
            HMODULE hModule = NULL;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)addr, &hModule)) {
                char modulePath[MAX_PATH];
                if (GetModuleFileNameA(hModule, modulePath, sizeof(modulePath))) {
                    return { std::filesystem::path(modulePath).filename().string(), addr - (DWORD64)hModule };
                }
            }
            return { "Unknown", addr };
            };

        auto [faultModule, faultOffset] = GetModuleInfo((DWORD64)record->ExceptionAddress);
        logFile << std::format("Lunar Tear version {}\n\n", LUNAR_TEAR_VERSION_STRING);
        logFile << std::format("Code: {:#010x}\n", record->ExceptionCode);
        logFile << std::format("Address: {} + {:#x}\n\n", faultModule, faultOffset);

        logFile << std::format("RAX: {:#018x}  RBX: {:#018x}  RCX: {:#018x}  RDX: {:#018x}\n", context->Rax, context->Rbx, context->Rcx, context->Rdx);
        logFile << std::format("RSI: {:#018x}  RDI: {:#018x}  RBP: {:#018x}  RSP: {:#018x}\n", context->Rsi, context->Rdi, context->Rbp, context->Rsp);
        logFile << std::format("R8:  {:#018x}  R9:  {:#018x}  R10: {:#018x}  R11: {:#018x}\n", context->R8, context->R9, context->R10, context->R11);
        logFile << std::format("R12: {:#018x}  R13: {:#018x}  R14: {:#018x}  R15: {:#018x}\n", context->R12, context->R13, context->R14, context->R15);
        logFile << std::format("RIP: {:#018x}\n\n", context->Rip);

        logFile << "Call Stack:\n";
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        // 1. Tell DbgHelp to clean up C++ names and load line data
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        SymInitialize(process, NULL, TRUE);

        STACKFRAME64 stackFrame = {};
        stackFrame.AddrPC.Offset = context->Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context->Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context->Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;

        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &stackFrame, context,
            NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {

            DWORD64 address = stackFrame.AddrPC.Offset;
            if (address == 0) break;

            auto [moduleName, offset] = GetModuleInfo(address);

            std::string symbolStr = "";
            DWORD64 displacement = 0;
            if (SymFromAddr(process, address, &displacement, symbol)) {
                symbolStr = std::format(" [{}]", symbol->Name);
            }

            std::string lineStr = "";
            IMAGEHLP_LINE64 line = { 0 };
            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
            DWORD displacement32 = 0; 

            if (SymGetLineFromAddr64(process, address, &displacement32, &line)) {
                lineStr = std::format(" ({}:{})", std::filesystem::path(line.FileName).filename().string(), line.LineNumber);
            }

            logFile << std::format("- {} + {:#x}{}{}\n", moduleName, offset, symbolStr, lineStr);
        }

        SymCleanup(process);
        logFile.close();
    }
    catch (...) {
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallCrashHandler() {
    AddVectoredExceptionHandler(1, VectoredExceptionHandler);
}