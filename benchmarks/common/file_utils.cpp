#include "file_utils.hpp"
#include <cstdio>  // for FILE, fopen, fclose, fgets
#include <new>     // for std::nothrow

namespace core {
namespace bench {

// Internal implementation (hidden from header)
struct FileHandle {
    std::FILE* fp;
};

FileHandle* OpenFileForReading(const char* path) noexcept {
    if (path == nullptr) {
        return nullptr;
    }

    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return nullptr;
    }

    FileHandle* handle = new (std::nothrow) FileHandle;
    if (handle == nullptr) {
        std::fclose(fp);
        return nullptr;
    }

    handle->fp = fp;
    return handle;
}

void CloseFile(FileHandle* handle) noexcept {
    if (handle != nullptr) {
        if (handle->fp != nullptr) {
            std::fclose(handle->fp);
        }
        delete handle;
    }
}

bool ReadLine(FileHandle* handle, char* buffer, usize bufferSize) noexcept {
    if (handle == nullptr || handle->fp == nullptr || buffer == nullptr || bufferSize == 0) {
        return false;
    }

    return std::fgets(buffer, static_cast<int>(bufferSize), handle->fp) != nullptr;
}

} // namespace bench
} // namespace core
