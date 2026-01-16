#pragma once
#include <string>
#include <vector>
#include <span>

#include "Logger.h"

using enum Logger::LogCategory;

inline std::string encodeBase16(std::span<const std::byte> input) {
    const char hex_chars[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(input.size() * 2);
    for (std::byte b : input) {
        uint8_t val = std::to_integer<uint8_t>(b);
        result.push_back(hex_chars[val >> 4]);
        result.push_back(hex_chars[val & 0x0F]);
    }
    return result;
}

inline std::vector<std::byte> decodeBase16(const std::string& hex) {
    if (hex.size() % 2 != 0) return {};

    std::vector<std::byte> output;
    output.reserve(hex.size() / 2);

     auto hexVal = [](char c) -> int8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
        };

    for (size_t i = 0; i < hex.size(); i += 2) {
        int8_t high = hexVal(hex[i]);
        int8_t low = hexVal(hex[i + 1]);
        if (high < 0 || low < 0) {
            Logger::Log(Error) << "base16 decode: Invalid hex string: `" << hex << "`";
            return {};
        }
        output.push_back(static_cast<std::byte>((high << 4) | low));
    }
    return output;
}