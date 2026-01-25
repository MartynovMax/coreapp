#include "operation_stream.hpp"
#include "core/base/core_assert.hpp"
#include "../common/seeded_rng.hpp"
#include <cmath>

namespace core {
namespace bench {

OperationStream::OperationStream(const WorkloadParams& params, SeededRNG& rng) noexcept
    : _params(params)
    , _rng(rng)
    , _currentOp(0)
{
    ASSERT(&_params != nullptr);
}

Operation OperationStream::Next() noexcept {
    ASSERT(_currentOp <= _params.operationCount);
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

core::memory_alignment OperationStream::GenerateAlignment(u32 size) noexcept {
    const AlignmentDistribution& ad = _params.alignmentDistribution;

    switch (ad.type) {
        case AlignmentDistributionType::Fixed:
            return ad.fixedAlignment; // 0 allowed => allocator default

        case AlignmentDistributionType::PowerOfTwoRange: {
            // Convert range into [minPower, maxPower]
            core::memory_alignment minA = ad.minAlignment;
            core::memory_alignment maxA = ad.maxAlignment;
            if (minA == 0) minA = 1;
            if (maxA < minA) maxA = minA;

            u32 minPower = 0;
            u32 maxPower = 0;

            core::memory_alignment t = minA;
            while (t > 1) { t >>= 1; minPower++; }

            t = maxA;
            while (t > 1) { t >>= 1; maxPower++; }

            const u32 power = _rng.NextRange(minPower, maxPower);
            return static_cast<core::memory_alignment>(1u << power);
        }

        case AlignmentDistributionType::MatchSizePow2: {
            core::memory_alignment a = NextPow2(static_cast<core::memory_alignment>(size));
            if (a < ad.minAlignment) a = ad.minAlignment;
            if (a > ad.maxAlignment) a = ad.maxAlignment;
            // allow 0? In this mode we want explicit result; if minAlignment is 0, clamp would keep 0,
            // but that would be weird. Keep it safe:
            if (a == 0) a = CORE_DEFAULT_ALIGNMENT;
            return a;
        }

        case AlignmentDistributionType::Typical64:
        case AlignmentDistributionType::CustomBuckets: {
            if (ad.buckets == nullptr || ad.weights == nullptr || ad.bucketCount == 0) {
                return ad.fixedAlignment; // fallback, can be 0
            }

            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            float cumulative = 0.0f;
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

u32 OperationStream::GenerateSize() noexcept {
    const SizeDistribution& dist = _params.sizeDistribution;

    switch (dist.type) {
        case DistributionType::Uniform:
            return _rng.NextRange(dist.minSize, dist.maxSize);

        case DistributionType::PowerOfTwo: {
            u32 minPower = 0;
            u32 maxPower = 0;

            u32 temp = dist.minSize;
            while (temp > 1) { temp >>= 1; minPower++; }

            temp = dist.maxSize;
            while (temp > 1) { temp >>= 1; maxPower++; }

            u32 power = _rng.NextRange(minPower, maxPower);
            return 1u << power;
        }

        case DistributionType::Normal: {
            float sum = 0.0f;
            for (int i = 0; i < 12; i++) {
                sum += _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            }
            float z = sum - 6.0f;
            float value = dist.mean + z * dist.stddev;

            if (value < static_cast<float>(dist.minSize)) return dist.minSize;
            if (value > static_cast<float>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::LogNormal: {
            float sum = 0.0f;
            for (int i = 0; i < 12; i++) {
                sum += _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            }
            float z = sum - 6.0f;
            float logValue = dist.mean + z * dist.stddev;
            float value = expf(logValue);

            if (value < static_cast<float>(dist.minSize)) return dist.minSize;
            if (value > static_cast<float>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::Exponential: {
            float u = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (u == 0.0f) u = 1e-8f;
            float lambda = (dist.shape > 0.0f) ? dist.shape : 1.0f;
            float value = -logf(u) / lambda;

            if (value < static_cast<float>(dist.minSize)) return dist.minSize;
            if (value > static_cast<float>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::Pareto: {
            float u = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (u == 0.0f) u = 1e-8f;
            float alpha = (dist.shape > 0.0f) ? dist.shape : 1.5f;
            float value = static_cast<float>(dist.minSize) * powf(1.0f / u, 1.0f / alpha);

            if (value < static_cast<float>(dist.minSize)) return dist.minSize;
            if (value > static_cast<float>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::SmallBiased: {
            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (r < 0.9f) {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _rng.NextRange(dist.minSize, smallMax);
            } else {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _rng.NextRange(largeMin, dist.maxSize);
            }
        }

        case DistributionType::LargeBiased: {
            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (r < 0.9f) {
                u32 largeMin = (dist.minSize + 64 < dist.maxSize) ? (dist.minSize + 64) : dist.minSize;
                return _rng.NextRange(largeMin, dist.maxSize);
            } else {
                u32 smallMax = (dist.minSize + 56 < dist.maxSize) ? (dist.minSize + 56) : dist.maxSize;
                return _rng.NextRange(dist.minSize, smallMax);
            }
        }

        case DistributionType::Bimodal: {
            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (r < dist.peak1Weight) return _rng.NextRange(dist.peak1Min, dist.peak1Max);
            return _rng.NextRange(dist.peak2Min, dist.peak2Max);
        }

        case DistributionType::WebServerAlloc: {
            float sum = 0.0f;
            for (int i = 0; i < 12; i++) {
                sum += _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            }
            float z = sum - 6.0f;
            float logValue = dist.mean + z * dist.stddev;
            float value = expf(logValue);

            if (value < static_cast<float>(dist.minSize)) return dist.minSize;
            if (value > static_cast<float>(dist.maxSize)) return dist.maxSize;
            return static_cast<u32>(value);
        }

        case DistributionType::GameEngine: {
            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            if (r < dist.peak1Weight) return _rng.NextRange(dist.peak1Min, dist.peak1Max);
            return _rng.NextRange(dist.peak2Min, dist.peak2Max);
        }

        case DistributionType::DatabaseCache: {
            u32 minPower = 0;
            u32 maxPower = 0;

            u32 temp = dist.minSize;
            while (temp > 1) { temp >>= 1; minPower++; }

            temp = dist.maxSize;
            while (temp > 1) { temp >>= 1; maxPower++; }

            u32 power = _rng.NextRange(minPower, maxPower);
            return 1u << power;
        }

        case DistributionType::CustomBuckets: {
            if (dist.buckets == nullptr || dist.weights == nullptr || dist.bucketCount == 0) {
                return _rng.NextRange(dist.minSize, dist.maxSize);
            }

            float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            float cumulative = 0.0f;

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

OpType OperationStream::DecideOperation() noexcept {
    float r = _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
    return (r < _params.allocFreeRatio) ? OpType::Alloc : OpType::Free;
}

} // namespace bench
} // namespace core
