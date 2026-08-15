#pragma once
#include "replicant/core/common.h"
#include <vector>
#include <string>
#include <cstdint>
#include <expected>
#include <array>
#include <span>

namespace replicant {

    struct KpkEntry {
        std::string name;
        std::vector<std::byte> data;
    };

    class KpkFile {
    public:
        std::array<char, 4> magic = { 'K', 'P', 'K', '\x7F' };
        uint32_t unknown = 0;
        std::vector<KpkEntry> entries;

        static std::expected<KpkFile, Error> Deserialize(std::span<const std::byte> data);
        std::expected<std::vector<std::byte>, Error> Serialize() const;

        KpkEntry* findFile(const std::string& name);
        const KpkEntry* findFile(const std::string& name) const;

    private:
        std::vector<std::byte> SerializeInternal() const;
    };

}