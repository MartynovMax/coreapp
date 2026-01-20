#pragma once

// =============================================================================
// seeded_rng.hpp
// Deterministic pseudo-random number generator for experiments.
//
// Uses xorshift64* algorithm for fast, deterministic generation.
// Same seed guarantees same sequence across platforms.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// SeededRNG - Deterministic random number generator
// ----------------------------------------------------------------------------

class SeededRNG {
public:
    explicit SeededRNG(u64 seed) noexcept;
    ~SeededRNG() noexcept = default;

    // Generate next random u32
    u32 NextU32() noexcept;

    // Generate next random u64
    u64 NextU64() noexcept;

    // Generate random value in range [min, max] (inclusive)
    u32 NextRange(u32 min, u32 max) noexcept;

private:
    u64 _state;
};

} // namespace bench
} // namespace core
