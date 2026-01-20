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
    const ExperimentDescriptor* GetAll(u32& out_count) const noexcept;

    // Filter experiments by pattern (returns count of matches)
    u32 Filter(const char* pattern, const ExperimentDescriptor** out_results, u32 max_results) const noexcept;

    // Get total count of registered experiments
    u32 Count() const noexcept;

    // Clear all registrations
    void Clear() noexcept;

private:
    static constexpr u32 kMaxExperiments = 256;
    
    ExperimentDescriptor experiments_[kMaxExperiments];
    u32 count_ = 0;
};

} // namespace bench
} // namespace core
