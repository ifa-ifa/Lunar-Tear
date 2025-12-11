#pragma once
#include <vector>
#include <string>

std::wstring BytesToHex(const unsigned char* data, size_t len);
bool HashFileSHA256(const std::wstring& path, std::vector<unsigned char>& outHash);
