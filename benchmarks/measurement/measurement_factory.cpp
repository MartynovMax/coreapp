#include "measurement_factory.hpp"
#include "../common/string_utils.hpp"

namespace core {
namespace bench {

MeasurementFactory::MeasurementFactory() noexcept
    : _systems{}, _systemCount(0) {
}

MeasurementFactory::~MeasurementFactory() noexcept {
    Clear();
}

u32 MeasurementFactory::ParseAndCreate(const char* measurementsList) noexcept {
    if (measurementsList == nullptr || *measurementsList == '\0') {
        return 0;
    }

    const char* start = measurementsList;
    const char* current = start;

    char nameBuffer[64];

    while (*current != '\0') {
        const char* end = current;
        while (*end != '\0' && *end != ',') {
            ++end;
        }

        u32 length = static_cast<u32>(end - current);
        if (length > 0 && length < sizeof(nameBuffer)) {
            for (u32 i = 0; i < length; ++i) {
                nameBuffer[i] = current[i];
            }
            nameBuffer[length] = '\0';

            const char* trimmed = nameBuffer;
            while (*trimmed == ' ' || *trimmed == '\t') {
                ++trimmed;
            }

            if (*trimmed != '\0') {
                IMeasurementSystem* system = CreateSystem(trimmed);
                if (system != nullptr && _systemCount < kMaxSystems) {
                    _systems[_systemCount++] = system;
                }
            }
        }

        if (*end == ',') {
            current = end + 1;
        } else {
            break;
        }
    }

    return _systemCount;
}

void MeasurementFactory::Clear() noexcept {
    for (u32 i = 0; i < _systemCount; ++i) {
        delete _systems[i];
        _systems[i] = nullptr;
    }
    _systemCount = 0;
}

IMeasurementSystem* MeasurementFactory::CreateSystem(const char* name) noexcept {
    if (name == nullptr) {
        return nullptr;
    }

    if (StringsEqual(name, "timer")) {
        return new TimerMeasurementSystem();
    }

    if (StringsEqual(name, "counter")) {
        return new CounterMeasurementSystem();
    }

    if (StringsEqual(name, "snapshot")) {
        return new SnapshotMeasurementSystem(1);
    }

    return nullptr;
}

} // namespace bench
} // namespace core
