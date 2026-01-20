#include "string_utils.hpp"

namespace core {
namespace bench {

bool StringsEqual(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return a == b;
    }
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

} // namespace bench
} // namespace core
