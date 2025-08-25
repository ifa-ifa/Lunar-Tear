#include<Windows.h>
#include <iostream>
#include<string>
#include <TlHelp32.h>

LPCWSTR app_path = L"NieR Replicant ver.1.22474487139.exe";
LPCWSTR dll_path = L"LunarTearLoader.dll";


DWORD GetProcessIdByName(const wchar_t* processName) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32FirstW(snapshot, &entry) == TRUE) {
        while (Process32NextW(snapshot, &entry) == TRUE) {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return 0;
}

std::string GetLastErrorAsString()
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return std::string();
    }
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
}

std::wstring stringToWide(const std::string& text) {
    std::wstring wide(text.length() + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], wide.size());
    return wide;
}

BOOL WINAPI main(int argc, char* argv[])
{

    if (GetFileAttributesW(dll_path) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(0, L"Error when launching process. Error message: dll not found", L"launcher error", 0);
        return FALSE;
    }

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi;
    BOOL ret = CreateProcessW(
        app_path,          // Application name
        NULL,              // Command line
        NULL,              // Process Attributes
        NULL,              // Thread Attributes
        FALSE,             // handle inheritance
        DETACHED_PROCESS,                 // creation flags
        NULL,              // environment
        NULL,              // starting directory
        &si,               // Startup info
        &pi                // Process Information
    );

    std::string error;

    if (!ret) {
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when launching process. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }

    DWORD stubProcessId = pi.dwProcessId;

    /*
    Can't directly use the handle returned from CreateProcess becuase it just calls 
    the steamapi and asks to be opened, then closes itself.
    */
    DWORD gameProcessId = 0;
    const int maxAttempts = 10000; 
    const int interval = 0;  

    std::wcout << L"Waiting for game process to appear..." << std::endl;
    for (int i = 0; i < maxAttempts; ++i) {
        DWORD foundProcessId = GetProcessIdByName(L"NieR Replicant ver.1.22474487139.exe");

        // If we found a process AND its PID is NOT the same as the stub's PID, we've found the real game.
        if (foundProcessId != 0 && foundProcessId != stubProcessId) {
            std::wcout << L"Real game process found! PID: " << foundProcessId << std::endl;
            gameProcessId = foundProcessId;
            break;
        }
        Sleep(interval);
    }


    if (gameProcessId == 0) {
        MessageBoxW(0, L"Error when opening window handle. Error message: PID is null.", L"launcher error", 0);
        return FALSE;
    }





    HANDLE process_handle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, gameProcessId);


    if (process_handle == NULL) {
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when opening handle. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }


    LPVOID lpBaseAddress = VirtualAllocEx(process_handle, NULL, (int64_t)1 << 12, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (lpBaseAddress == NULL) {
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when allocating memory in target process. Error message: ") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }

    if (!WriteProcessMemory(process_handle, lpBaseAddress, (LPCVOID)(dll_path), (wcslen(dll_path) + 1) * sizeof(wchar_t), nullptr)) {
        VirtualFreeEx(process_handle, lpBaseAddress, 0, MEM_RELEASE);
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when overwriting arguments in target process. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"Kernel32");
    if (hKernel32 == NULL) {
        VirtualFreeEx(process_handle, lpBaseAddress, 0x0, MEM_RELEASE);
        lpBaseAddress = NULL;

        CloseHandle(process_handle);
        process_handle = NULL;
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when loading dll into target process. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }
    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (pLoadLibraryW == NULL) {
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when finding address of LoadLibraryW. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }


    HANDLE thread_handle = CreateRemoteThread(process_handle, NULL, NULL, (LPTHREAD_START_ROUTINE)pLoadLibraryW, lpBaseAddress, NULL, NULL);
    if (thread_handle == NULL) {
        VirtualFreeEx(process_handle, lpBaseAddress, 0x0, MEM_RELEASE);
        lpBaseAddress = nullptr;

        CloseHandle(process_handle);
        process_handle = NULL;

        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when creating remote thread. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }
    WaitForSingleObject(thread_handle, 5000);

    VirtualFreeEx(process_handle, lpBaseAddress, 0, MEM_RELEASE);
    CloseHandle(thread_handle);
    CloseHandle(process_handle);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return TRUE;
}