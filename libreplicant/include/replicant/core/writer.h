#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <map>
#include <span>

namespace replicant {

    class Writer {
        std::vector<std::byte> buffer_;

    public:
        Writer() = default;

        explicit Writer(size_t reserved_size) { buffer_.reserve(reserved_size); }

        const std::vector<std::byte>& buffer() const { return buffer_; }
        size_t tell() const { return buffer_.size(); }

        template <typename T>
        void write(const T& value) {
            const std::byte* ptr = reinterpret_cast<const std::byte*>(&value);
            buffer_.insert(buffer_.end(), ptr, ptr + sizeof(T));
        }

        void write(const void* data, size_t size) {
            const std::byte* ptr = reinterpret_cast<const std::byte*>(data);
            buffer_.insert(buffer_.end(), ptr, ptr + size);
        }

        void align(size_t alignment) {
            size_t current = tell();
            size_t padding = (alignment - (current % alignment)) % alignment;
            for (size_t i = 0; i < padding; i++) buffer_.push_back(std::byte{ 0 });
        }

        size_t reserveOffset() {
            size_t pos = tell();
            write<uint32_t>(0);
            return pos;
        }

        void satisfyOffset(size_t patch_loc, size_t target_pos) {
            if (patch_loc + sizeof(uint32_t) > buffer_.size()) {
                throw std::out_of_range("Writer::satisfyOffset: Patch location is out of buffer bounds");
            }

            int64_t relative = static_cast<int64_t>(target_pos) - static_cast<int64_t>(patch_loc);

            uint32_t val = static_cast<uint32_t>(relative);
            std::memcpy(buffer_.data() + patch_loc, &val, sizeof(uint32_t));
        }

        void satisfyOffsetHere(size_t patch_loc) {
            satisfyOffset(patch_loc, tell());
        }
    };


    class StringPool {
        std::map<std::string, std::vector<size_t>> pending_patches_;

    public:

        void add(const std::string& str, size_t patch_loc) {
            if (str.empty()) return; 
            pending_patches_[str].push_back(patch_loc);
        }

        void flush(Writer& writer) {
            for (const auto& [str, patches] : pending_patches_) {
                size_t string_start_pos = writer.tell();

                writer.write(str.c_str(), str.length() + 1);

                for (size_t patch_loc : patches) {
                    writer.satisfyOffset(patch_loc, string_start_pos);
                }
            }
            pending_patches_.clear();
        }
    };
}