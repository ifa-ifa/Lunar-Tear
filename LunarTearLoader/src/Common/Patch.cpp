#include <Windows.h>
#include "Patch.h"

    Patch::Patch(uintptr_t addr, std::vector<uint8_t> patch)
        : address((void*)addr), patchBytes(std::move(patch)) {
    }

    void Patch::Apply() {
        DWORD oldProtect;

        if (originalBytes.empty()) {
            originalBytes.resize(patchBytes.size());
            memcpy(originalBytes.data(), address, patchBytes.size());
        }

        VirtualProtect(address, patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(address, patchBytes.data(), patchBytes.size());
        VirtualProtect(address, patchBytes.size(), oldProtect, &oldProtect);
    }

    void Patch::Revert() {
        if (originalBytes.empty()) return;

        DWORD oldProtect;

        VirtualProtect(address, originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(address, originalBytes.data(), originalBytes.size());
        VirtualProtect(address, originalBytes.size(), oldProtect, &oldProtect);
    }
