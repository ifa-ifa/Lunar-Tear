#include "Game/Globals.h"
#include "d3d11.h"

// ALL OF this is crap - TODO: Fix it


 void* GetD3D11Device() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->d3d11Device;

}

 void* GetD3D11DeviceContext() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->d3d11DeviceContext;

}

 void* GetDXGIFactory() {

    if (!rhiDevicePtr) {
        return nullptr;
    }

    if (!*rhiDevicePtr) {
        return nullptr;
    }

    return (*rhiDevicePtr)->dxgiFactory;

}

 void* GetPresentAddress() {
    if (!rhiSwapchainPtr) {
        return nullptr;
    }

    tpRhiSwapchain* pRhiSwapchain = *rhiSwapchainPtr;
    if (!pRhiSwapchain) {
        return nullptr;
    }

    void* pDxgiSwapchain = (void*)pRhiSwapchain->dxgiSwapchain;
    if (!pDxgiSwapchain) {
        return nullptr;
    }

    void** pVTable = *(void***)pDxgiSwapchain;
    if (!pVTable) {
        return nullptr;
    }

    return pVTable[8];
}

 void* GetDrawIndexedAddress() {
    return (*(void***)GetD3D11DeviceContext())[12];
}