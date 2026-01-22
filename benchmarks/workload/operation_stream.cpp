#include "operation_stream.hpp"
#include "../common/seeded_rng.hpp"

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

} // namespace bench
} // namespace core
