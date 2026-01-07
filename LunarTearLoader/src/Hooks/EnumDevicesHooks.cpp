// The implementation of this framerate unlocker, including patches and addresses, 
// is largely based of the Radical Replicant plugin for SpecialK by Kaldaien. The 
// specific plugin code was released under the MIT license, a copy of which can 
// be found below:
// 
// 
//
// Copyright 2021-2024 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include "Hooks/Hooks.h"
#include "Common/Logger.h"
#include "Common/Settings.h"
#include "Common/Patch.h"
#include "Game/Globals.h"
#include "Game/dinput8.h"
#include <Windows.h>
#include <vector>
#include <dbt.h> 
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

using enum Logger::LogCategory;

namespace {
    typedef HRESULT(WINAPI* EnumDevices_t)(IDirectInput8W*, DWORD, LPDIENUMDEVICESCALLBACKW, LPVOID, DWORD);
    EnumDevices_t EnumDevices_original = nullptr;

    std::vector<DIDEVICEINSTANCEW> g_cachedDevices;
    std::mutex g_deviceCacheMutex;
    bool g_isFirstEnum = true;
    bool g_deviceListDirty = false;

    BOOL CALLBACK CachingEnumCallback(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef) {

        auto* deviceVector = static_cast<std::vector<DIDEVICEINSTANCEW>*>(pvRef);

        // Lets only add devices that are actual game controllers
        // I think Special K does this to prevent issues with other devices like driving / flying controllers
        // that may report themselves as game controllers (I dont have any to test this...)
        if (lpddi->dwDevType & DI8DEVTYPE_GAMEPAD || lpddi->dwDevType & DI8DEVTYPE_JOYSTICK) {
            Logger::Log(Info) << "DirectInput: Found device '" << lpddi->tszProductName << "'";
            deviceVector->push_back(*lpddi);
        }
        else {
            Logger::Log(Verbose) << "DirectInput: Skipping non-gamepad device '" << lpddi->tszProductName << "'";
        }

        return DIENUM_CONTINUE;
    }

}

HRESULT WINAPI EnumDevices_detoured(IDirectInput8W* pThis, DWORD dwDevType, LPDIENUMDEVICESCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags) {

    // We only want to intercept the enumeration for game controllers
    // let other enumerations pass through untouched
    if (dwDevType != DI8DEVCLASS_GAMECTRL) {
        return EnumDevices_original(pThis, dwDevType, lpCallback, pvRef, dwFlags);
    }

    std::lock_guard<std::mutex> lock(g_deviceCacheMutex);

    if (g_isFirstEnum || g_deviceListDirty) {
        Logger::Log(Verbose) << "DirectInput: Re-enumerating devices to build cache...";
        g_cachedDevices.clear();

        HRESULT hr = EnumDevices_original(pThis, dwDevType, CachingEnumCallback, &g_cachedDevices, dwFlags);

        if (SUCCEEDED(hr)) {
            Logger::Log(Verbose) << "DirectInput: Cache built with " << g_cachedDevices.size() << " devices";
            g_isFirstEnum = false;
            g_deviceListDirty = false;
        }
        else {
            Logger::Log(Error) << "DirectInput: Original EnumDevices call failed with HRESULT " << hr;
            return hr; 
        }
    }

    for (const auto& device : g_cachedDevices) {
        if (lpCallback(&device, pvRef) == DIENUM_STOP) {
            break; 
        }
    }

    return DI_OK;
}
    LRESULT CALLBACK DeviceNotificationWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_DEVICECHANGE) {
            if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                Logger::Log(Info) << "DirectInput: Device change detected. Invalidating device cache.";
                std::lock_guard<std::mutex> lock(g_deviceCacheMutex);
                g_deviceListDirty = true;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

void DeviceNotificationThread() {
    const wchar_t* CLASS_NAME = L"LunarTearDeviceNotifier";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (message == WM_DEVICECHANGE) {
            if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
                Logger::Log(Info) << "DirectInput: Device change detected. Invalidating device cache.";
                std::lock_guard<std::mutex> lock(g_deviceCacheMutex);
                g_deviceListDirty = true;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    };
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClassW(&wc)) {
        Logger::Log(Error) << "Failed to register window class for device notifications. Error: " << GetLastError();
        return;
    }

    HWND hWnd = CreateWindowExW(0, CLASS_NAME, L"Device Notifier", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (hWnd == NULL) {
        Logger::Log(Error) << "Failed to create device notification window. Error: " << GetLastError();
        return;
    }

    DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = { 0x4d1e55b2, 0xf16f, 0x11cf, { 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

    HDEVNOTIFY hNotify = RegisterDeviceNotificationW(hWnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (hNotify == NULL) {
        Logger::Log(Error) << "Failed to register for device notifications. Error: " << GetLastError();
    } else {
        Logger::Log(Verbose) << "Successfully registered for device notifications.";
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


bool InstallEnumDevicesHooks() {


    if (!Settings::Instance().FixDeviceEnumeration) {
        return true;
    }

    std::thread(DeviceNotificationThread).detach();

    void* pEnumDevices = nullptr;
    int timewaited = 0;
    while (!pEnumDevices) {
        if (timewaited > 10000) {
            Logger::Log(Error) << "Could not get Enum Devices address";
            return false;
        }
        pEnumDevices = GetEnumDevicesAddress();
        Sleep(100);
        timewaited += 100;
    }

    if (MH_CreateHookEx(pEnumDevices, &EnumDevices_detoured, &EnumDevices_original) != MH_OK) {
        Logger::Log(Error) << "Could not create hook for EnumDevices.";
        return false;
    }
    if (MH_EnableHook(pEnumDevices) != MH_OK) { 
        Logger::Log(Error) << "Could not enable device enumeration hook";
        return false;
    }

    Logger::Log(Verbose) << "Enum Devices hooks installed successfully.";
    return true;

}