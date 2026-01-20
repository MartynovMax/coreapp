#include "event_bus.hpp"

namespace core {
namespace bench {

void EventBus::Attach(IEventSink* sink) noexcept {
    if (sink == nullptr || _count >= kMaxSinks) {
        return;
    }
    _sinks[_count++] = sink;
}

void EventBus::Emit(const Event& event) noexcept {
    if (_count == 0) {
        return;
    }

    for (u32 i = 0; i < _count; ++i) {
        if (_sinks[i] != nullptr) {
            _sinks[i]->OnEvent(event);
        }
    }
}

} // namespace bench
} // namespace core
