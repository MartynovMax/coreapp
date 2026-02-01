#include "operation_stream.hpp"
#include "core/base/core_assert.hpp"
#include "../common/seeded_rng.hpp"
#include <cmath>

namespace core::bench {

namespace {
    // Internal helper for generating random floats in [0, 1) using core types
    inline f32 NextUnitFloat01(SeededRNG& rng) noexcept {
        constexpr f64 kInv2Pow32 = 1.0 / 4294967296.0; // 2^32
        return static_cast<f32>(static_cast<f64>(rng.NextU32()) * kInv2Pow32);
    }
}

OperationStream::OperationStream(const WorkloadParams& params, SeededRNG& rng) noexcept
    : _params(params)
    , _rng(rng)
    , _currentOp(0)
{
    ASSERT(_params.sizeDistribution.minSize > 0);
    ASSERT(_params.sizeDistribution.maxSize >= _params.sizeDistribution.minSize);
    ASSERT(_params.alignmentDistribution.maxAlignment >= _params.alignmentDistribution.minAlignment);
    if (_params.sizeDistribution.type == DistributionType::Bimodal || _params.sizeDistribution.type == DistributionType::GameEngine) {
        ASSERT(_params.sizeDistribution.peak1Min >= _params.sizeDistribution.minSize);
        ASSERT(_params.sizeDistribution.peak1Max <= _params.sizeDistribution.maxSize);
        ASSERT(_params.sizeDistribution.peak2Min >= _params.sizeDistribution.minSize);
        ASSERT(_params.sizeDistribution.peak2Max <= _params.sizeDistribution.maxSize);
    }
    if (_params.sizeDistribution.type == DistributionType::CustomBuckets) {
        ASSERT(_params.sizeDistribution.bucketCount > 0);
        ASSERT(_params.sizeDistribution.buckets != nullptr);
        ASSERT(_params.sizeDistribution.weights != nullptr);
        f32 sum = 0.0f;
        for (u32 i = 0; i < _params.sizeDistribution.bucketCount; ++i) sum += _params.sizeDistribution.weights[i];
        ASSERT(sum > 0.99f && sum < 1.01f);
    }
    if (_params.alignmentDistribution.type == AlignmentDistributionType::CustomBuckets) {
        ASSERT(_params.alignmentDistribution.bucketCount > 0);
        ASSERT(_params.alignmentDistribution.buckets != nullptr);
        ASSERT(_params.alignmentDistribution.weights != nullptr);
        f32 sum = 0.0f;
        for (u32 i = 0; i < _params.alignmentDistribution.bucketCount; ++i) sum += _params.alignmentDistribution.weights[i];
        ASSERT(sum > 0.99f && sum < 1.01f);
    }
}

Operation OperationStream::Next() noexcept {
    ASSERT(_currentOp < _params.operationCount);
    Operation op{};
    op.type = DecideOperation();

    if (op.type == OpType::Alloc) {
        op.size = GenerateSize();
        op.alignment = GenerateAlignment(op.size);
        op.tag = _params.tag;
        op.flags = _params.flags;
        op.ptr = nullptr;
    } else {
        op.size = 0;
        op.alignment = 0;
        op.tag = 0;
        op.flags = core::AllocationFlags::None;
        op.ptr = nullptr;
    }

    _currentOp++;
    return op;
}

Operation OperationStream::Next(u64 liveCount) noexcept {
    ASSERT(_currentOp < _params.operationCount);
    Operation op{};

    // Decide operation based on liveCount and allocFreeRatio
    if (liveCount == 0) {
        op.type = (_params.allocFreeRatio > 0.0f) ? OpType::Alloc : OpType::Free;
    } else {
        op.type = DecideOperation();
    }

    if (op.type == OpType::Alloc) {
        op.size = GenerateSize();
        op.alignment = GenerateAlignment(op.size);
        op.tag = _params.tag;
        op.flags = _params.flags;
        op.ptr = nullptr;
    } else {
        op.size = 0;
        op.alignment = 0;
        op.tag = 0;
        op.flags = core::AllocationFlags::None;
        op.ptr = nullptr;
    }

    _currentOp++;
    return op;
}

bool OperationStream::HasNext() const noexcept {
    return _currentOp < _params.operationCount;
}

core::memory_alignment OperationStream::NextPow2(core::memory_alignment v) noexcept {
    if (v <= 1) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

core::memory_alignment OperationStream::GenerateAlignment(u32 size) const noexcept {
    const AlignmentDistribution& ad = _params.alignmentDistribution;

    switch (ad.type) {
        case AlignmentDistributionType::Fixed:
            return ad.fixedAlignment; // 0 allowed => allocator default

        case AlignmentDistributionType::PowerOfTwoRange: {
            core::memory_alignment minA = NextPow2(ad.minAlignment);
            core::memory_alignment maxA = NextPow2(ad.maxAlignment);
            if (maxA == 0) maxA = minA;
            if (minA > maxA) minA = maxA;
            u32 minPower = 0, maxPower = 0;
            core::memory_alignment t = minA;
            while (t > 1) { t >>= 1; minPower++; }
            t = maxA;
            while (t > 1) { t >>= 1; maxPower++; }
            if (minPower > maxPower) minPower = maxPower;
            const u32 power = _rng.NextRange(minPower, maxPower);
            return static_cast<core::memory_alignment>(1u << power);
        }

        case AlignmentDistributionType::MatchSizePow2: {
            core::memory_alignment a = NextPow2(static_cast<core::memory_alignment>(size));
            if (a < ad.minAlignment) a = ad.minAlignment;
            if (a > ad.maxAlignment) a = ad.maxAlignment;
            if (a == 0) a = CORE_DEFAULT_ALIGNMENT;
            return a;
        }

        case AlignmentDistributionType::Typical64: {
            static const core::memory_alignment defBuckets[4] = {8, 16, 32, 64};
            static const f32 defWeights[4] = {0.2f, 0.6f, 0.15f, 0.05f};
            const core::memory_alignment* buckets = ad.buckets ? ad.buckets : defBuckets;
            const f32* weights = ad.weights ? ad.weights : defWeights;
            u32 count = ad.buckets && ad.weights && ad.bucketCount > 0 ? ad.bucketCount : 4;
            if (count > 4) count = 4;
            f32 r = NextUnitFloat01(_rng);
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < count; i++) {
                cumulative += weights[i];
                if (r < cumulative) return buckets[i];
            }
            return buckets[count - 1];
        }
        case AlignmentDistributionType::CustomBuckets: {
            if (ad.buckets == nullptr || ad.weights == nullptr || ad.bucketCount == 0) {
                return ad.fixedAlignment; // fallback, can be 0
            }
            f32 r = NextUnitFloat01(_rng);
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < ad.bucketCount; i++) {
                cumulative += ad.weights[i];
                if (r < cumulative) {
                    return ad.buckets[i];
                }
            }
            return ad.buckets[ad.bucketCount - 1];
        }

        default:
            return ad.fixedAlignment;
    }
}

u32 OperationStream::GenerateSize() const noexcept {
    const SizeDistribution& dist = _params.sizeDistribution;

    // Clamp min/max for all distributions
    ASSERT(dist.minSize > 0);
    ASSERT(dist.maxSize >= dist.minSize);

    switch (dist.type) {
        case DistributionType::Uniform:
            return _rng.NextRange(dist.minSize, dist.maxSize);

        case DistributionType::PowerOfTwo: {
            u32 minPower = 0;
            u32 temp = dist.minSize;
            if (temp > 1 && (temp & (temp - 1)) != 0) {
                while (temp > 1) { temp >>= 1; minPower++; }
                minPower++;
            } else {
                while (temp > 1) { temp >>= 1; minPower++; }
            }
            u32 maxPower = 0;
            temp = dist.maxSize;
            while (temp > 1) { temp >>= 1; maxPower++; }
            u32 power = _rng.NextRange(minPower, maxPower);
            u32 sz = 1u << power;
            if (sz < dist.minSize) sz = dist.minSize;
            if (sz > dist.maxSize) sz = dist.maxSize;
            return sz;
        }

        case DistributionType::Normal: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01(_rng);
            }
            f32 z = sum - 6.0f;
            f32 value = dist.mean + z * dist.stddev;

            if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
            if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::LogNormal: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01(_rng);
            }
            f32 z = sum - 6.0f;
            f32 logValue = dist.mean + z * dist.stddev;
            f32 value = expf(logValue);

            if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
            if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::Exponential: {
            f32 u = NextUnitFloat01(_rng);
            if (u == 0.0f) u = 1e-8f;
            f32 lambda = (dist.shape > 0.0f) ? dist.shape : 1.0f;
            f32 value = -logf(u) / lambda;

            if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
            if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::Pareto: {
            f32 u = NextUnitFloat01(_rng);
            if (u == 0.0f) u = 1e-8f;
            f32 alpha = (dist.shape > 0.0f) ? dist.shape : 1.5f;
            f32 value = static_cast<f32>(dist.minSize) * powf(1.0f / u, 1.0f / alpha);

            if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
            if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::SmallBiased: {
            f32 r = NextUnitFloat01(_rng);
            if (r < 0.9f) {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _rng.NextRange(dist.minSize, smallMax);
            } else {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _rng.NextRange(largeMin, dist.maxSize);
            }
        }

        case DistributionType::LargeBiased: {
            f32 r = NextUnitFloat01(_rng);
            if (r < 0.9f) {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _rng.NextRange(largeMin, dist.maxSize);
            } else {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _rng.NextRange(dist.minSize, smallMax);
            }
        }

        case DistributionType::Bimodal: {
            u32 peak1Min = dist.peak1Min < dist.minSize ? dist.minSize : dist.peak1Min;
            u32 peak1Max = dist.peak1Max > dist.maxSize ? dist.maxSize : dist.peak1Max;
            u32 peak2Min = dist.peak2Min < dist.minSize ? dist.minSize : dist.peak2Min;
            u32 peak2Max = dist.peak2Max > dist.maxSize ? dist.maxSize : dist.peak2Max;
            f32 r = NextUnitFloat01(_rng);
            if (r < dist.peak1Weight) return _rng.NextRange(peak1Min, peak1Max);
            return _rng.NextRange(peak2Min, peak2Max);
        }

        case DistributionType::WebServerAlloc: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01(_rng);
            }
            f32 z = sum - 6.0f;
            f32 logValue = dist.mean + z * dist.stddev;
            f32 value = expf(logValue);

            if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
            if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::GameEngine: {
            u32 peak1Min = dist.peak1Min < dist.minSize ? dist.minSize : dist.peak1Min;
            u32 peak1Max = dist.peak1Max > dist.maxSize ? dist.maxSize : dist.peak1Max;
            u32 peak2Min = dist.peak2Min < dist.minSize ? dist.minSize : dist.peak2Min;
            u32 peak2Max = dist.peak2Max > dist.maxSize ? dist.maxSize : dist.peak2Max;
            f32 r = NextUnitFloat01(_rng);
            if (r < dist.peak1Weight) return _rng.NextRange(peak1Min, peak1Max);
            return _rng.NextRange(peak2Min, peak2Max);
        }

        case DistributionType::DatabaseCache: {
            u32 minPower = 0;
            u32 temp = dist.minSize;
            if (temp > 1 && (temp & (temp - 1)) != 0) {
                while (temp > 1) { temp >>= 1; minPower++; }
                minPower++;
            } else {
                while (temp > 1) { temp >>= 1; minPower++; }
            }
            u32 maxPower = 0;
            temp = dist.maxSize;
            while (temp > 1) { temp >>= 1; maxPower++; }
            u32 power = _rng.NextRange(minPower, maxPower);
            u32 sz = 1u << power;
            if (sz < dist.minSize) sz = dist.minSize;
            if (sz > dist.maxSize) sz = dist.maxSize;
            return sz;
        }

        case DistributionType::CustomBuckets: {
            if (dist.buckets == nullptr || dist.weights == nullptr || dist.bucketCount == 0) {
                return _rng.NextRange(dist.minSize, dist.maxSize);
            }
            f32 r = NextUnitFloat01(_rng);
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < dist.bucketCount; i++) {
                cumulative += dist.weights[i];
                if (r < cumulative) return dist.buckets[i];
            }
            return dist.buckets[dist.bucketCount - 1];
        }

        default:
            return _rng.NextRange(dist.minSize, dist.maxSize);
    }
}

OpType OperationStream::DecideOperation() const noexcept {
    f32 ratio = _params.allocFreeRatio;

    // Handle NaN and clamp
    if (!(ratio >= 0.0f)) ratio = 0.0f; // covers NaN and negatives
    if (ratio > 1.0f) ratio = 1.0f;

    // Strict endpoints
    if (ratio <= 0.0f) return OpType::Free;
    if (ratio >= 1.0f) return OpType::Alloc;

    const f32 r = NextUnitFloat01(_rng); // [0,1)
    return (r < ratio) ? OpType::Alloc : OpType::Free;
}

} // namespace core::bench
