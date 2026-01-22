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

} // namespace bench
} // namespace core
