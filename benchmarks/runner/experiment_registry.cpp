#include "experiment_registry.hpp"
#include "../common/pattern_matcher.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

bool ExperimentRegistry::Register(const ExperimentDescriptor& descriptor) noexcept {
    if (_count >= kMaxExperiments) {
        return false; // Registry full
    }
    _experiments[_count++] = descriptor;
    return true;
}

const ExperimentDescriptor* ExperimentRegistry::Find(const char* name) const noexcept {
    for (u32 i = 0; i < _count; ++i) {
        if (StringsEqual(_experiments[i].name, name)) {
            return &_experiments[i];
        }
    }
    return nullptr;
}

const ExperimentDescriptor* ExperimentRegistry::GetAll(u32& outCount) const noexcept {
    outCount = _count;
    return (_count > 0) ? _experiments : nullptr;
}

u32 ExperimentRegistry::Filter(const char* pattern, const ExperimentDescriptor** outResults, u32 maxResults) const noexcept {
    if (pattern == nullptr || outResults == nullptr || maxResults == 0) {
        return 0;
    }
    
    u32 matchCount = 0;
    for (u32 i = 0; i < _count && matchCount < maxResults; ++i) {
        if (PatternMatch(pattern, _experiments[i].name)) {
            outResults[matchCount++] = &_experiments[i];
        }
    }
    
    return matchCount;
}

u32 ExperimentRegistry::Count() const noexcept {
    return _count;
}

void ExperimentRegistry::Clear() noexcept {
    _count = 0;
}

} // namespace bench
} // namespace core
