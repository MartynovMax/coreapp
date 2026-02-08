#include "core/base/core_types.hpp"
#include "operation_stream.hpp"
#include "core/base/core_assert.hpp"
#include "../common/seeded_rng.hpp"
#include <cmath>

namespace core::bench {

namespace {
    inline f32 NormalizeAllocFreeRatio(f32 ratio) noexcept {
        if (!(ratio >= 0.0f)) return 0.0f;
        if (ratio > 1.0f) return 1.0f;
        return ratio;
    }
    inline bool IsPowerOfTwo(core::memory_alignment v) noexcept {
        if (v == 0) return true;
        return (v & (v - 1)) == 0;
    }
    
    inline u32 ClampSize(f32 value, const SizeDistribution& dist) noexcept {
        if (value < static_cast<f32>(dist.minSize)) return dist.minSize;
        if (value > static_cast<f32>(dist.maxSize)) return dist.maxSize;
        return static_cast<u32>(value);
    }

    inline core::memory_alignment NextPow2Align(core::memory_alignment v) noexcept {
        if (v == 0) return 1;
        if ((v & (v - 1)) == 0) return v;
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        if constexpr (sizeof(core::memory_alignment) >= 2) v |= v >> 8;
        if constexpr (sizeof(core::memory_alignment) >= 4) v |= v >> 16;
        if constexpr (sizeof(core::memory_alignment) >= 8) v |= v >> 32;
        v++;
        return v;
    }
}

OperationStream::OperationStream(const WorkloadParams& params) noexcept
    : _params(params)
    , _ownedRng(params.seed)
    , _currentOp(0)
    , _normalizedAlignmentBucketCount(0)
{
    _params.allocFreeRatio = NormalizeAllocFreeRatio(_params.allocFreeRatio);

    if (_params.sizeDistribution.minSize == 0) {
        _params.sizeDistribution.minSize = 1;
    }
    if (_params.sizeDistribution.maxSize < _params.sizeDistribution.minSize) {
        u32 temp = _params.sizeDistribution.minSize;
        _params.sizeDistribution.minSize = _params.sizeDistribution.maxSize;
        _params.sizeDistribution.maxSize = temp;
    }

    ASSERT(_params.sizeDistribution.minSize > 0);
    ASSERT(_params.sizeDistribution.maxSize >= _params.sizeDistribution.minSize);

    if (_params.alignmentDistribution.maxAlignment < _params.alignmentDistribution.minAlignment) {
        core::memory_alignment temp = _params.alignmentDistribution.minAlignment;
        _params.alignmentDistribution.minAlignment = _params.alignmentDistribution.maxAlignment;
        _params.alignmentDistribution.maxAlignment = temp;
    }
    ASSERT(_params.alignmentDistribution.maxAlignment >= _params.alignmentDistribution.minAlignment);

    if (_params.sizeDistribution.type == DistributionType::Bimodal || _params.sizeDistribution.type == DistributionType::GameEngine) {
        auto& sd = _params.sizeDistribution;
        
        if (sd.peak1Min < sd.minSize) sd.peak1Min = sd.minSize;
        if (sd.peak1Max > sd.maxSize) sd.peak1Max = sd.maxSize;
        if (sd.peak1Min > sd.maxSize) sd.peak1Min = sd.maxSize;
        if (sd.peak1Max < sd.minSize) sd.peak1Max = sd.minSize;
        if (sd.peak1Max < sd.peak1Min) {
            u32 temp = sd.peak1Min;
            sd.peak1Min = sd.peak1Max;
            sd.peak1Max = temp;
        }
        
        if (sd.peak2Min < sd.minSize) sd.peak2Min = sd.minSize;
        if (sd.peak2Max > sd.maxSize) sd.peak2Max = sd.maxSize;
        if (sd.peak2Min > sd.maxSize) sd.peak2Min = sd.maxSize;
        if (sd.peak2Max < sd.minSize) sd.peak2Max = sd.minSize;
        if (sd.peak2Max < sd.peak2Min) {
            u32 temp = sd.peak2Min;
            sd.peak2Min = sd.peak2Max;
            sd.peak2Max = temp;
        }

        ASSERT(_params.sizeDistribution.peak1Min >= _params.sizeDistribution.minSize);
        ASSERT(_params.sizeDistribution.peak1Max <= _params.sizeDistribution.maxSize);
        ASSERT(_params.sizeDistribution.peak2Min >= _params.sizeDistribution.minSize);
        ASSERT(_params.sizeDistribution.peak2Max <= _params.sizeDistribution.maxSize);
        ASSERT(_params.sizeDistribution.peak1Min <= _params.sizeDistribution.peak1Max);
        ASSERT(_params.sizeDistribution.peak2Min <= _params.sizeDistribution.peak2Max);
    }

    if (_params.sizeDistribution.type == DistributionType::CustomBuckets) {
        ASSERT(_params.sizeDistribution.bucketCount > 0);
        ASSERT(_params.sizeDistribution.buckets != nullptr);
        ASSERT(_params.sizeDistribution.weights != nullptr);
        f32 sum = 0.0f;
        for (u32 i = 0; i < _params.sizeDistribution.bucketCount; ++i) sum += _params.sizeDistribution.weights[i];
        ASSERT(sum > 0.99f && sum < 1.01f);
        for (u32 i = 0; i < _params.sizeDistribution.bucketCount; ++i) {
            u32 sz = _params.sizeDistribution.buckets[i];
            ASSERT(sz > 0 && "CustomBuckets: size must be > 0");
            ASSERT(sz >= _params.sizeDistribution.minSize && 
                   sz <= _params.sizeDistribution.maxSize &&
                   "CustomBuckets: size out of [minSize, maxSize] range");
        }
    }

    if (_params.alignmentDistribution.type == AlignmentDistributionType::CustomBuckets) {
        ASSERT(_params.alignmentDistribution.bucketCount > 0);
        ASSERT(_params.alignmentDistribution.buckets != nullptr);
        ASSERT(_params.alignmentDistribution.weights != nullptr);
        
        _normalizedAlignmentBucketCount = _params.alignmentDistribution.bucketCount;
        if (_normalizedAlignmentBucketCount > kMaxAlignmentBuckets) {
            _normalizedAlignmentBucketCount = kMaxAlignmentBuckets;
        }
        
        for (u32 i = 0; i < _normalizedAlignmentBucketCount; ++i) {
            core::memory_alignment a = _params.alignmentDistribution.buckets[i];
            if (!IsPowerOfTwo(a) || a == 0) {
                a = NextPow2Align(a);
                if (a == 0) a = CORE_DEFAULT_ALIGNMENT;
            }
            _normalizedAlignmentBuckets[i] = a;
        }

        f32 sum = 0.0f;
        for (u32 i = 0; i < _params.alignmentDistribution.bucketCount; ++i) sum += _params.alignmentDistribution.weights[i];
        ASSERT(sum > 0.99f && sum < 1.01f);
    }
    
    if (_params.alignmentDistribution.type == AlignmentDistributionType::PowerOfTwoRange) {
        ASSERT(IsPowerOfTwo(_params.alignmentDistribution.minAlignment) && 
               "PowerOfTwoRange requires minAlignment to be power-of-2 (or 0)");
        ASSERT(IsPowerOfTwo(_params.alignmentDistribution.maxAlignment) && 
               "PowerOfTwoRange requires maxAlignment to be power-of-2 (or 0)");
    }
}

Operation OperationStream::Next(u64 liveCount) noexcept {
    ASSERT(_currentOp < _params.operationCount);
    Operation op{};
    op.reason = OpReason::Normal;

    if (liveCount == 0) {
        op.type = DecideOperation();
        if (op.type == OpType::Free) {
            if (_params.allocFreeRatio > 0.0f) {
                op.type = OpType::Alloc;
                op.reason = OpReason::ForcedAllocEmptyLive;
            } else {
                op.reason = OpReason::NoopFreeEmptyLive;
            }
        }
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
    if (v == 0) return 0; // Explicitly handle zero case
    if (v <= 1) return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if constexpr (sizeof(core::memory_alignment) > 4) {
        v |= v >> 32; // Handle 64-bit alignment
    }
    v++;
    return v;
}

core::memory_alignment OperationStream::GenerateAlignment(u32 size) noexcept {
    const AlignmentDistribution& ad = _params.alignmentDistribution;

    switch (ad.type) {
        case AlignmentDistributionType::Fixed:
            return ad.fixedAlignment; // 0 allowed => allocator default

        case AlignmentDistributionType::PowerOfTwoRange: {
            core::memory_alignment minA = ad.minAlignment;
            core::memory_alignment maxA = ad.maxAlignment;
            
            if (minA == 0) minA = CORE_DEFAULT_ALIGNMENT;
            if (maxA == 0) maxA = CORE_DEFAULT_ALIGNMENT;
            
            // Both are guaranteed to be pow2 already
            if (minA > maxA) minA = maxA;

            u32 minPower = 0, maxPower = 0;
            core::memory_alignment t = minA;
            while (t > 1) { t >>= 1; minPower++; }
            t = maxA;
            while (t > 1) { t >>= 1; maxPower++; }

            u32 power = _ownedRng.NextRange(minPower, maxPower);

            constexpr u32 kBitWidth = static_cast<u32>(sizeof(core::memory_alignment) * 8u);
            ASSERT(kBitWidth > 0);
            ASSERT(power < kBitWidth);
            if (power >= kBitWidth) {
                power = kBitWidth - 1;
            }

            const core::memory_alignment one = static_cast<core::memory_alignment>(1);
            core::memory_alignment result = static_cast<core::memory_alignment>(one << power);
            
            return result;
        }

        case AlignmentDistributionType::MatchSizePow2: {
            core::memory_alignment a = NextPow2(static_cast<core::memory_alignment>(size));
            core::memory_alignment minA = ad.minAlignment;
            core::memory_alignment maxA = ad.maxAlignment;
            
            if (minA == 0) minA = CORE_DEFAULT_ALIGNMENT;
            if (maxA == 0) maxA = CORE_DEFAULT_ALIGNMENT;
            
            if (minA > maxA) {
                core::memory_alignment temp = minA;
                minA = maxA;
                maxA = temp;
            }
            
            if (a < minA) a = minA;
            if (a > maxA) a = maxA;
            
            return a;
        }

        case AlignmentDistributionType::Typical64: {
            static core::memory_alignment defBuckets[4] = {8, 16, 32, 64};
            static const f32 defWeights[4] = {0.2f, 0.6f, 0.15f, 0.05f};
            const core::memory_alignment* buckets = ad.buckets ? ad.buckets : defBuckets;
            const f32* weights = ad.weights ? ad.weights : defWeights;
            u32 count = ad.buckets && ad.weights && ad.bucketCount > 0 ? ad.bucketCount : 4;
            if (count > 4) count = 4;
            f32 r = NextUnitFloat01();
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < count; i++) {
                cumulative += weights[i];
                if (r < cumulative) {
                    core::memory_alignment result = buckets[i];
                    if (!IsPowerOfTwo(result) || result == 0) {
                        result = NextPow2Align(result);
                        if (result == 0) result = CORE_DEFAULT_ALIGNMENT;
                    }
                    return result;
                }
            }
            core::memory_alignment result = buckets[count - 1];
            if (!IsPowerOfTwo(result) || result == 0) {
                result = NextPow2Align(result);
                if (result == 0) result = CORE_DEFAULT_ALIGNMENT;
            }
            return result;
        }
        case AlignmentDistributionType::CustomBuckets: {
            if (_normalizedAlignmentBucketCount == 0) {
                return ad.fixedAlignment ? ad.fixedAlignment : CORE_DEFAULT_ALIGNMENT;
            }
            f32 r = NextUnitFloat01();
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < _normalizedAlignmentBucketCount; i++) {
                cumulative += ad.weights[i];
                if (r < cumulative) {
                    return _normalizedAlignmentBuckets[i];
                }
            }
            return _normalizedAlignmentBuckets[_normalizedAlignmentBucketCount - 1];
        }

        default:
            return ad.fixedAlignment;
    }
}

u32 OperationStream::GenerateSize() noexcept {
    const SizeDistribution& dist = _params.sizeDistribution;

    // Clamp min/max for all distributions
    ASSERT(dist.minSize > 0);
    ASSERT(dist.maxSize >= dist.minSize);


    constexpr u32 kBitWidthU32 = static_cast<u32>(sizeof(u32) * 8u);

    switch (dist.type) {
        case DistributionType::Uniform:
            return _ownedRng.NextRange(dist.minSize, dist.maxSize);

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

            u32 power = _ownedRng.NextRange(minPower, maxPower);

            ASSERT(power < kBitWidthU32);
            if (power >= kBitWidthU32) {
                power = kBitWidthU32 - 1;
            }

            u32 sz = 1u << power;
            if (sz < dist.minSize) sz = dist.minSize;
            if (sz > dist.maxSize) sz = dist.maxSize;
            return sz;
        }

        case DistributionType::Normal: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01();
            }
            f32 z = sum - 6.0f;
            f32 value = dist.mean + z * dist.stddev;
            return ClampSize(value, dist);
        }

        case DistributionType::LogNormal: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01();
            }
            f32 z = sum - 6.0f;
            
            f32 mu, sigma;
            if (dist.meanInLogSpace) {
                // Parameters already in log-space
                mu = dist.mean;
                sigma = dist.stddev;
            } else {
                f32 meanLin = dist.mean;
                f32 stddevLin = dist.stddev;
                
                constexpr f32 kEpsilon = 1e-6f;
                if (meanLin < kEpsilon) meanLin = 1.0f;
                if (stddevLin < 0.0f) stddevLin = 0.0f;
                
                f32 cv = stddevLin / meanLin; // Coefficient of variation
                
                if (!(cv >= 0.0f && cv < 1e6f)) { 
                    cv = 0.0f;
                }
                
                f32 sigmaSquared = logf(1.0f + cv * cv);
                sigma = sqrtf(sigmaSquared);
                mu = logf(meanLin) - 0.5f * sigmaSquared;
            }
            
            f32 logValue = mu + z * sigma;
            f32 value = expf(logValue);
            return ClampSize(value, dist);
        }

        case DistributionType::Exponential: {
            f32 u = NextUnitFloat01();
            if (u == 0.0f) u = 1e-8f;
            f32 lambda = (dist.shape > 0.0f) ? dist.shape : 1.0f;
            f32 value = -logf(u) / lambda;
            return ClampSize(value, dist);
        }

        case DistributionType::Pareto: {
            f32 u = NextUnitFloat01();
            if (u == 0.0f) u = 1e-8f;
            f32 alpha = (dist.shape > 0.0f) ? dist.shape : 1.5f;
            f32 value = static_cast<f32>(dist.minSize) * powf(1.0f / u, 1.0f / alpha);
            return ClampSize(value, dist);
        }

        case DistributionType::SmallBiased: {
            f32 r = NextUnitFloat01();
            if (r < 0.9f) {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _ownedRng.NextRange(dist.minSize, smallMax);
            } else {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _ownedRng.NextRange(largeMin, dist.maxSize);
            }
        }

        case DistributionType::LargeBiased: {
            f32 r = NextUnitFloat01();
            if (r < 0.9f) {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _ownedRng.NextRange(largeMin, dist.maxSize);
            } else {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _ownedRng.NextRange(dist.minSize, smallMax);
            }
        }

        case DistributionType::Bimodal: {
            u32 peak1Min = dist.peak1Min < dist.minSize ? dist.minSize : dist.peak1Min;
            u32 peak1Max = dist.peak1Max > dist.maxSize ? dist.maxSize : dist.peak1Max;
            u32 peak2Min = dist.peak2Min < dist.minSize ? dist.minSize : dist.peak2Min;
            u32 peak2Max = dist.peak2Max > dist.maxSize ? dist.maxSize : dist.peak2Max;
            f32 r = NextUnitFloat01();
            if (r < dist.peak1Weight) return _ownedRng.NextRange(peak1Min, peak1Max);
            return _ownedRng.NextRange(peak2Min, peak2Max);
        }

        case DistributionType::WebServerAlloc: {
            f32 sum = 0.0f;
            for (i32 i = 0; i < 12; i++) {
                sum += NextUnitFloat01();
            }
            f32 z = sum - 6.0f;
            f32 logValue = dist.mean + z * dist.stddev;
            f32 value = expf(logValue);
            return ClampSize(value, dist);
        }

        case DistributionType::GameEngine: {
            u32 peak1Min = dist.peak1Min < dist.minSize ? dist.minSize : dist.peak1Min;
            u32 peak1Max = dist.peak1Max > dist.maxSize ? dist.maxSize : dist.peak1Max;
            u32 peak2Min = dist.peak2Min < dist.minSize ? dist.minSize : dist.peak2Min;
            u32 peak2Max = dist.peak2Max > dist.maxSize ? dist.maxSize : dist.peak2Max;
            f32 r = NextUnitFloat01();
            if (r < dist.peak1Weight) return _ownedRng.NextRange(peak1Min, peak1Max);
            return _ownedRng.NextRange(peak2Min, peak2Max);
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

            u32 power = _ownedRng.NextRange(minPower, maxPower);

            ASSERT(power < kBitWidthU32);
            if (power >= kBitWidthU32) {
                power = kBitWidthU32 - 1;
            }

            u32 sz = 1u << power;
            if (sz < dist.minSize) sz = dist.minSize;
            if (sz > dist.maxSize) sz = dist.maxSize;
            return sz;
        }

        case DistributionType::CustomBuckets: {
            if (dist.buckets == nullptr || dist.weights == nullptr || dist.bucketCount == 0) {
                return _ownedRng.NextRange(dist.minSize, dist.maxSize);
            }
            f32 r = NextUnitFloat01();
            f32 cumulative = 0.0f;
            for (u32 i = 0; i < dist.bucketCount; i++) {
                cumulative += dist.weights[i];
                if (r < cumulative) return dist.buckets[i];
            }
            return dist.buckets[dist.bucketCount - 1];
        }

        default:
            return _ownedRng.NextRange(dist.minSize, dist.maxSize);
    }
}

OpType OperationStream::DecideOperation() noexcept {
    f32 ratio = _params.allocFreeRatio;

    if (ratio <= 0.0f) return OpType::Free;
    if (ratio >= 1.0f) return OpType::Alloc;

    const f32 r = NextUnitFloat01();
    return (r < ratio) ? OpType::Alloc : OpType::Free;
}

f32 OperationStream::NextUnitFloat01() noexcept {
    constexpr f64 kInv2Pow32 = 1.0 / 4294967296.0;
    return static_cast<f32>(static_cast<f64>(_ownedRng.NextU32()) * kInv2Pow32);
}

} // namespace core::bench
