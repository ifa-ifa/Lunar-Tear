#include "Game/Globals.h"
#include <Windows.h>

// ALL OF this is crap - TODO: Fix it

inline void* GetD3D11Device() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->d3d11Device;

}

inline void* GetD3D11DeviceContext() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->d3d11DeviceContext;

}

inline void* GetDXGIFactory() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->dxgiFactory;

}

#include <dxgi.h>
using enum Logger::LogCategory;

inline void* GetPresentAddress() {

    

    uint8_t** prswap = (uint8_t**)rhiSwapchainPtr;

    void*** swap = (void***)(*prswap + 0x78);

    void** swapvtable = (void**)**swap;

    return swapvtable[8];
}

inline void* GetDrawIndexedAddress() {


    return (*(void***)GetD3D11DeviceContext())[12];
}