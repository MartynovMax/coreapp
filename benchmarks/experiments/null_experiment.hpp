#pragma once

// =============================================================================
// null_experiment.hpp
// Minimal no-op experiment for testing runner infrastructure.
// =============================================================================

#include "../runner/experiment_interface.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// NullExperiment - Does nothing (for testing)
// ----------------------------------------------------------------------------

class NullExperiment final : public IExperiment {
public:
    NullExperiment() noexcept = default;
    ~NullExperiment() noexcept override = default;

    void Setup(const ExperimentParams& params) override;
    void Warmup() override;
    void RunPhases(u32 repetitionIndex) override;
    void Teardown() noexcept override;

    const char* Name() const noexcept override;
    const char* Category() const noexcept override;
    const char* Description() const noexcept override;
    const char* AllocatorName() const noexcept override;

    // Factory function for registration
    static IExperiment* Create() noexcept;
};

} // namespace bench
} // namespace core
