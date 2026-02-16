#pragma once

// =============================================================================
// measurement_factory.hpp
// Factory for creating measurement systems from CLI configuration.
// =============================================================================

#include "measurement_system.hpp"
#include "timer_system.hpp"
#include "counter_system.hpp"
#include "snapshot_system.hpp"
#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

class MeasurementFactory {
public:
    static constexpr u32 kMaxSystems = 16;

    MeasurementFactory() noexcept;
    ~MeasurementFactory() noexcept;

    u32 ParseAndCreate(const char* measurementsList) noexcept;

    [[nodiscard]] u32 GetSystemCount() const noexcept { return _systemCount; }
    [[nodiscard]] IMeasurementSystem** GetSystems() noexcept { return _systems; }

    void Clear() noexcept;

private:
    IMeasurementSystem* _systems[kMaxSystems];
    u32 _systemCount;

    IMeasurementSystem* CreateSystem(const char* name) noexcept;
};

} // namespace bench
} // namespace core
