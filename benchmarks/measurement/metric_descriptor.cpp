#include "metric_descriptor.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

bool MetricDescriptor::RequiresCapability(const char* cap) const noexcept {
    if (capability == nullptr || cap == nullptr) {
        return false;
    }
    return StringsEqual(capability, cap);
}

} // namespace bench
} // namespace core
