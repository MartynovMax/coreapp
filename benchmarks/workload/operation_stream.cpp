#include "operation_stream.hpp"
#include "../common/seeded_rng.hpp"
#include <cmath>

namespace core {
namespace bench {

OperationStream::OperationStream(const WorkloadParams& params, SeededRNG& rng) noexcept
    : _params(params)
    , _rng(rng)
    , _currentOp(0)
{
}

Operation OperationStream::Next() noexcept {
    Operation op = {};
    op.type = DecideOperation();
    
    if (op.type == OpType::Alloc) {
        op.size = GenerateSize();
        op.ptr = nullptr;
    } else {
        op.size = 0;
        op.ptr = nullptr;
    }
    
    _currentOp++;
    return op;
}

bool OperationStream::HasNext() const noexcept {
    return _currentOp < _params.operationCount;
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
            while (temp > 1) {
                temp >>= 1;
                minPower++;
            }
            
            temp = dist.maxSize;
            while (temp > 1) {
                temp >>= 1;
                maxPower++;
            }
            
            u32 power = _rng.NextRange(minPower, maxPower);
            return 1u << power;
        }
        
        case DistributionType::Normal: {
            // Use sum of uniform random variables (Central Limit Theorem)
            // Sum of 12 uniform [0,1] gives mean=6, stddev≈1
            float sum = 0.0f;
            for (int i = 0; i < 12; i++) {
                sum += _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            }
            float z = sum - 6.0f;
            float value = dist.mean + z * dist.stddev;
            
            if (value < static_cast<float>(dist.minSize)) {
                return dist.minSize;
            }
            if (value > static_cast<float>(dist.maxSize)) {
                return dist.maxSize;
            }
            return static_cast<u32>(value);
        }
        
        case DistributionType::LogNormal: {
            // LogNormal = exp(Normal)
            float sum = 0.0f;
            for (int i = 0; i < 12; i++) {
                sum += _rng.NextU32() / static_cast<float>(0xFFFFFFFFu);
            }
            float z = sum - 6.0f;
            float logValue = dist.mean + z * dist.stddev;
            float value = expf(logValue);
            
            if (value < static_cast<float>(dist.minSize)) {
                return dist.minSize;
            }
            if (value > static_cast<float>(dist.maxSize)) {
                return dist.maxSize;
            }
            return static_cast<u32>(value);
        }
        
        default:
            return _rng.NextRange(dist.minSize, dist.maxSize);
    }
}

} // namespace bench
} // namespace core
