#include <vector>
#include <fstream>
#include <filesystem>
#include <span>
#include <expected>
#include <string>
#include <cstddef>

namespace replicant {

    enum class IOErrorCode {
        Success,
        FileNotFound,
        FileReadError,
        FileSizeError,
        MemoryAllocationError,
        DirectoryCreationError,
        FileWriteError,
        FileExists
    };

    struct IOError {
        IOErrorCode code;
        std::string message;
    };

    inline std::expected<std::vector<std::byte>, IOError> ReadFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            return std::unexpected(IOError{ IOErrorCode::FileNotFound, "File not found: " + path.string() });
        }

        auto fileSize = std::filesystem::file_size(path, ec);
        if (ec) {
            return std::unexpected(IOError{ IOErrorCode::FileSizeError, "Could not get file size: " + path.string() });
        }

        if (fileSize == 0) {
            return std::vector<std::byte>{};
        }

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return std::unexpected(IOError{ IOErrorCode::FileReadError, "Failed to open file: " + path.string() });
        }

        std::vector<std::byte> buffer;
        try {
            buffer.resize(fileSize);
        }
        catch (const std::bad_alloc&) {
            return std::unexpected(IOError{ IOErrorCode::MemoryAllocationError, "Failed to allocate memory for file: " + path.string() });
        }

        if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {

            if (file.gcount() != static_cast<std::streamsize>(fileSize)) {
                return std::unexpected(IOError{ IOErrorCode::FileReadError, "Failed to read expected bytes from: " + path.string() });
            }
        }

        return buffer;
    }

    inline std::expected<void, IOError> WriteFile(const std::filesystem::path& path, std::span<const std::byte> data, bool overwrite = true)
    {

        if (!overwrite) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) {
                if (ec) {
                    return std::unexpected(IOError{
                        IOErrorCode::FileWriteError,
                        "Failed to check existence of file: " + path.string()
                        });
                }
                return std::unexpected(IOError{
                    IOErrorCode::FileExists,
                    "File already exists and overwrite is disabled: " + path.string()
                    });
            }
        }

        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) {
                return std::unexpected(IOError{
                    IOErrorCode::DirectoryCreationError,
                    "Failed to create directories for: " + path.string()
                    });
            }
        }

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) {
            return std::unexpected(IOError{
                IOErrorCode::FileWriteError,
                "Failed to open file for writing (check permissions): " + path.string()
                });
        }

        if (!data.empty()) {
            file.write(reinterpret_cast<const char*>(data.data()), data.size());

            if (!file) {
                return std::unexpected(IOError{
                   IOErrorCode::FileWriteError,
                   "Stream error while writing (disk full?): " + path.string()
                    });
            }
        }

        return {};
    }


}