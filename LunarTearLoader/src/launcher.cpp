#include<Windows.h>
#include<string>
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


LPCWSTR app_path = L"NieR Replicant ver.1.22474487139.exe";
LPCWSTR dll_path = L"LunarTearLoader.dll";


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



    /*
    Can't directly use the handle returned from CreateProcess becuase that process isn't the game, its
    a short lived process that at some point creates the game process. Therefore we wait for the actual
    game window to load then hook to that
    */
    HWND window_handle = NULL;
    const int maxAttempts = 40;
    const int interval = 500;
    for (int i = 0; i < maxAttempts; i++) {
        window_handle = FindWindowW(NULL, L"NieR Replicant ver.1.22474487139...");
        if (window_handle) break;
        Sleep(interval);
    }


    if (!window_handle) {
        MessageBoxW(0, L"Error when opening window handle. Error message: Handle is null.", L"launcher error", 0);
        return FALSE;
    }


    DWORD process_id;
    GetWindowThreadProcessId(window_handle, &process_id);
    if (process_id == NULL) {
        error = GetLastErrorAsString();
        MessageBoxW(0, (std::wstring(L"Error when getting window thread process id. Error message:") + stringToWide(error)).c_str(), L"launcher error", 0);
        return FALSE;
    }


    HANDLE process_handle = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, process_id);


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