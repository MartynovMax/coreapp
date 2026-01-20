#pragma once

// =============================================================================
// experiment_registry.hpp
// Registry for experiment registration and lookup.
// =============================================================================

#include "experiment_descriptor.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// ExperimentRegistry - Manages registered experiments
// ----------------------------------------------------------------------------

class ExperimentRegistry {
public:
    ExperimentRegistry() noexcept = default;
    ~ExperimentRegistry() noexcept = default;

    ExperimentRegistry(const ExperimentRegistry&) = delete;
    ExperimentRegistry& operator=(const ExperimentRegistry&) = delete;

    // Register new experiment
    void Register(const ExperimentDescriptor& descriptor) noexcept;

    // Find experiment by exact name (returns nullptr if not found)
    const ExperimentDescriptor* Find(const char* name) const noexcept;

    // Get all registered experiments
    const ExperimentDescriptor* GetAll(u32& outCount) const noexcept;

    // Filter experiments by pattern (returns count of matches)
    u32 Filter(const char* pattern, const ExperimentDescriptor** outResults, u32 maxResults) const noexcept;

    // Get total count of registered experiments
    u32 Count() const noexcept;

    // Clear all registrations
    void Clear() noexcept;

private:
    static constexpr u32 kMaxExperiments = 256;
    
    ExperimentDescriptor _experiments[kMaxExperiments];
    u32 _count = 0;
};

} // namespace bench
} // namespace core
