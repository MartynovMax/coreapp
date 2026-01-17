#include "thread_local_arena.hpp"

namespace core {

CORE_API IArena& GetThreadLocalArena() noexcept {
    static IArena* dummy = nullptr;
    return *dummy;
}

CORE_API bool HasThreadLocalArena() noexcept {
    return false;
}

CORE_API void ResetThreadLocalArena() noexcept {
}

} // namespace core

