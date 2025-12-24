#pragma once
#include <vector>
#include <string>
#include <span>
#include <cstdint>
#include <cstring>
#include <expected> 
#include <string_view>

namespace replicant {

    enum class ReaderErrorCode {
        OutOfBounds,
        InvalidPointer, 
        InvalidOffset,  
        StringError     
    };

    struct ReaderError {
        ReaderErrorCode code;
        std::string message; 
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
        std::expected<const T*, ReaderError> view() {
            if (current_ + sizeof(T) > end_) [[unlikely]] {
                return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "Buffer overrun viewing struct" });
            }
            const T* ptr = reinterpret_cast<const T*>(current_);
            current_ += sizeof(T);
            return ptr;
        }

        template <typename T>
        std::expected<std::span<const T>, ReaderError> viewArray(size_t count) {
            if (count > 0 && sizeof(T) > (size_t)(end_ - current_) / count) [[unlikely]] {
                return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "Buffer overrun viewing array (size calc)" });
            }

            size_t bytes = count * sizeof(T);
            if (current_ + bytes > end_) [[unlikely]] {
                return std::unexpected(ReaderError{ ReaderErrorCode::OutOfBounds, "Buffer overrun viewing array" });
            }
            const T* ptr = reinterpret_cast<const T*>(current_);
            current_ += bytes;
            return std::span<const T>(ptr, count);
        }

        std::expected<const char*, ReaderError> getOffsetPtr(const uint32_t& offsetField) const {
            const char* fieldAddress = reinterpret_cast<const char*>(&offsetField);

            if (fieldAddress < start_ || fieldAddress >= end_) [[unlikely]] {
                return std::unexpected(ReaderError{ ReaderErrorCode::InvalidPointer, "Offset source field is not within reader buffer" });
            }

            const char* target = fieldAddress + offsetField;

            if (target < start_ || target >= end_) [[unlikely]] {
                return std::unexpected(ReaderError{ ReaderErrorCode::InvalidOffset, "Relative offset points outside buffer" });
            }
            return target;
        }

        std::expected<std::string, ReaderError> readStringRelative(const uint32_t& offsetField) const {
            if (offsetField == 0) return "";

            auto ptrResult = getOffsetPtr(offsetField);
            if (!ptrResult) return std::unexpected(ptrResult.error());

            const char* ptr = *ptrResult;

            size_t maxLen = end_ - ptr;
            size_t len = strnlen(ptr, maxLen);

            return std::string(ptr, len);
        }
    };
}