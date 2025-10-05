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
#include "Game/d3d11.h"
#include <Windows.h>
#include <vector>

using enum Logger::LogCategory;


namespace {
    typedef HRESULT(WINAPI* Present_t)(void* pSwapChain, uint32_t SyncInterval, uint32_t Flags);
    Present_t Present_original = nullptr;

    int fps_cap = 60;
    std::chrono::nanoseconds targetFrameDuration = std::chrono::nanoseconds(1000000000 / fps_cap);
}

HRESULT WINAPI Present_detoured(void* pSwapChain, uint32_t SyncInterval, uint32_t Flags) {

    static auto nextFrameTime = std::chrono::high_resolution_clock::now();

    if (inLoadingScreen != nullptr && *inLoadingScreen != 0) {
        return Present_original(pSwapChain, 1, Flags); 
    }

    if (fps_cap > 0) {
        auto now = std::chrono::high_resolution_clock::now();

        if (nextFrameTime < now) {
            nextFrameTime = now;
        }

        std::this_thread::sleep_until(nextFrameTime);

        nextFrameTime += targetFrameDuration;
    }

    return Present_original(pSwapChain, 0, Flags);
}

bool InstallFPSUnlockHooks() {

    fps_cap = Settings::Instance().FPS_Cap;

    if (fps_cap < 0) {
        return true;
    }

    else if (fps_cap > 0) {
        targetFrameDuration = std::chrono::nanoseconds(1000000000 / fps_cap);
        Logger::Log(Info) << "FPS cap set to: " << fps_cap;
    }
    else {
        Logger::Log(Info) << "FPS cap is disabled (unlimited).";
    }   


    Patch dtFixPatch(g_processBaseAddress + 0x6938B, { 0x81 });
    Patch rotationDetourPatch(g_processBaseAddress + 0x66E580, { 0xE9, 0x6B, 0x8D, 0xE0, 0xFF, 0x90, 0x90, 0x90 });
    Patch rotationCavePatch(g_processBaseAddress + 0x4772F0, { 0xF3, 0x0F, 0x10, 0x0D, 0xBC, 0x7D, 0x8D, 0x00, 0xF3, 0x0F,
                                                             0x5E, 0x0D, 0x40, 0x60, 0x6A, 0x04, 0xF3, 0x0F, 0x59, 0xD1, 
                                                             0xE9, 0x7F, 0x72, 0x1F, 0x00, 0x90 });

    dtFixPatch.Apply();
    rotationDetourPatch.Apply();
    rotationCavePatch.Apply();


    void* pPresent = nullptr;
    int timewaited = 0;
    while (!pPresent) {
        if (timewaited > 10000) {
            Logger::Log(Error) << "Could not get Present address. FPS limiter will not work";
            return false;
        }
        pPresent = GetPresentAddress();
        Sleep(100);
        timewaited += 100;
    }

    if (MH_CreateHookEx(pPresent, &Present_detoured, &Present_original) != MH_OK) {
        Logger::Log(Error) << "Could not create hook for IDXGISwapChain::Present.";
        return false;
    }
    

    Logger::Log(Info) << "FPS unlock hooks installed successfully.";
    return true;
}