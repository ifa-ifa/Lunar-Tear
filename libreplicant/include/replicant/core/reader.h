#pragma once
#include <vector>
#include <string>
#include <span>
#include <cstdint>
#include <cstring>
#include <stdexcept> 
#include <string_view>

namespace replicant {

    class ReaderException : public std::runtime_error {
    public:
        explicit ReaderException(const std::string& message)
            : std::runtime_error(message) {}
    };

    class Reader {
        const char* start_;
        const char* end_;
        const char* current_;

    public:

        explicit Reader(std::span<const char> data)
            : start_(data.data()), end_(data.data() + data.size()), current_(data.data()) {
        }

        explicit Reader(std::span<const std::byte> data)
            : start_(reinterpret_cast<const char*>(data.data())),
            end_(reinterpret_cast<const char*>(data.data() + data.size())),
            current_(reinterpret_cast<const char*>(data.data())) {
        }

        template <typename T>
        const T* view() {
            if (current_ + sizeof(T) > end_) [[unlikely]] {
				throw(ReaderException("Buffer overrun viewing struct"));
            }
            const T* ptr = reinterpret_cast<const T*>(current_);
            current_ += sizeof(T);
            return ptr;
        }

        template <typename T>
        std::span<const T> viewArray(size_t count) {
            if (count > 0 && sizeof(T) > (size_t)(end_ - current_) / count) [[unlikely]] {
				throw(ReaderException("Buffer overrun viewing array (size calc)"));
            }

            size_t bytes = count * sizeof(T);
            if (current_ + bytes > end_) [[unlikely]] {
				throw(ReaderException("Buffer overrun viewing array"));
            }
            const T* ptr = reinterpret_cast<const T*>(current_);
            current_ += bytes;
            return std::span<const T>(ptr, count);
        }

        void seek(const void* ptr) {
            const char* target = static_cast<const char*>(ptr);
            if (target < start_ || target > end_) {
				throw(ReaderException("Seek target is outside reader buffer"));
            }
            current_ = target;
            return;
        }

        size_t remaining() const {
            return end_ - current_;
		}

        const void* currentPtr() const {
            return current_;
		}

        const char* getOffsetPtr(const uint32_t& offsetField) const {
            const char* fieldAddress = reinterpret_cast<const char*>(&offsetField);

            if (fieldAddress < start_ || fieldAddress >= end_) [[unlikely]] {
				throw(ReaderException("Field address is outside buffer"));
            }

            const char* target = fieldAddress + offsetField;

            if (target < start_ || target >= end_) [[unlikely]] {
				throw(ReaderException("Computed offset pointer is outside buffer"));
            }
            return target;
        }

        std::string readStringRelative(const uint32_t& offsetField) const {
            if (offsetField == 0) return "";

            auto ptr = getOffsetPtr(offsetField);

            size_t maxLen = end_ - ptr;
            size_t len = strnlen(ptr, maxLen);

            return std::string(ptr, len);
        }
    };
}