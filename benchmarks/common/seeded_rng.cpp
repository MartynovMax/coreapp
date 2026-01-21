#include "seeded_rng.hpp"

namespace core {
namespace bench {

SeededRNG::SeededRNG(u64 seed) noexcept
    : _state(seed)
{
    if (_state == 0) {
        _state = 1;
    }
}

u64 SeededRNG::NextU64() noexcept {
    u64 x = _state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    _state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

u32 SeededRNG::NextU32() noexcept {
    return static_cast<u32>(NextU64());
}

u32 SeededRNG::NextRange(u32 min, u32 max) noexcept {
    if (min >= max) {
        return min;
    }
    
    u64 range = static_cast<u64>(max) - static_cast<u64>(min) + 1;
    return min + static_cast<u32>(NextU32() % range);
}

} // namespace bench
} // namespace core
