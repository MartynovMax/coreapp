#pragma once

// =============================================================================
// experiment_interface.hpp
// Base interface for benchmark experiments.
//
// Lifecycle: Setup() → Warmup() × N → RunPhases() × M → Teardown()
//
// Determinism:
//   - Workload must be reproducible given seed
//   - Timing is NOT deterministic (subject to system noise)
//
// Thread-safety:
//   - Single-threaded execution (no concurrent lifecycle calls)
// =============================================================================

namespace core {
namespace bench {

// Forward declarations
struct ExperimentParams;
class IEventSink;

// ----------------------------------------------------------------------------
// IExperiment - Base interface for all experiments
// ----------------------------------------------------------------------------

class IExperiment {
public:
    virtual ~IExperiment() = default;
    
    // Lifecycle: called in order: Setup → Warmup × N → RunPhases × M → Teardown
    virtual void Setup(const ExperimentParams& params) noexcept = 0;
    virtual void Warmup() noexcept = 0;
    virtual void RunPhases() noexcept = 0;
    virtual void Teardown() noexcept = 0;
    
    // Metadata for filtering and reporting
    virtual const char* Name() const noexcept = 0;
    virtual const char* Category() const noexcept = 0;
    virtual const char* Description() const noexcept = 0;
    virtual const char* AllocatorName() const noexcept = 0;
    
    // Optional event integration (default: no-op)
    virtual void AttachEventSink(IEventSink* sink) noexcept {
        (void)sink;
    }
};

} // namespace bench
} // namespace core
