#include "SHA256.h"

#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <filesystem>
#include <format>

#pragma comment(lib, "bcrypt.lib")

std::wstring BytesToHex(const unsigned char* data, size_t len) {
    std::wstring result;
    result.reserve(len * 2);

    for (size_t i = 0; i < len; ++i)
        result += std::format(L"{:02X}", data[i]);

    return result;
}

bool HashFileSHA256(const std::wstring& path, std::vector<unsigned char>& outHash) {
    outHash.clear();

    std::ifstream file(std::filesystem::path(path), std::ios::binary);
    if (!file) {
        MessageBoxW(NULL,
            (L"LunarTear: failed to open file:\n" + path).c_str(),
            L"Hash error",
            MB_OK);
        return false;
    }

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);

    if (status != 0) {
        MessageBoxW(NULL, L"BCryptOpenAlgorithmProvider failed", L"Hash error", MB_OK);
        return false;
    }

    DWORD hashLen = 0, objLen = 0, cbResult = 0;

    status = BCryptGetProperty(
        hAlg, BCRYPT_HASH_LENGTH,
        (PUCHAR)&hashLen, sizeof(hashLen),
        &cbResult, 0
    );

    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        MessageBoxW(NULL, L"BCryptGetProperty HASH_LENGTH failed", L"Hash error", MB_OK);
        return false;
    }

    status = BCryptGetProperty(
        hAlg, BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&objLen, sizeof(objLen),
        &cbResult, 0
    );

    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        MessageBoxW(NULL, L"BCryptGetProperty OBJECT_LENGTH failed", L"Hash error", MB_OK);
        return false;
    }

    outHash.resize(hashLen);
    std::vector<unsigned char> hashObject(objLen);

    BCRYPT_HASH_HANDLE hHash = nullptr;
    status = BCryptCreateHash(
        hAlg, &hHash,
        hashObject.data(), (ULONG)hashObject.size(),
        nullptr, 0, 0
    );

    if (status != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        MessageBoxW(NULL, L"BCryptCreateHash failed", L"Hash error", MB_OK);
        return false;
    }

    char buffer[8192];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        status = BCryptHashData(
            hHash,
            (PUCHAR)buffer,
            (ULONG)file.gcount(),
            0
        );

        if (status != 0) {
            BCryptDestroyHash(hHash);
            BCryptCloseAlgorithmProvider(hAlg, 0);
            MessageBoxW(NULL, L"BCryptHashData failed", L"Hash error", MB_OK);
            return false;
        }
    }

    status = BCryptFinishHash(
        hHash,
        outHash.data(),
        hashLen,
        0
    );

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != 0) {
        MessageBoxW(NULL, L"BCryptFinishHash failed", L"Hash error", MB_OK);
        return false;
    }

    return true;
}
