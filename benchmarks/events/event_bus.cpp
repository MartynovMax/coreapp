#include "event_bus.hpp"
#include "../../core/concurrency/atomic_u64.hpp"

namespace core {
namespace bench {

static atomic_u64 g_nextEventSeqNo{1};

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

    Event eventWithSeq = event;
    eventWithSeq.eventSeqNo = g_nextEventSeqNo.fetch_add(1, memory_order::relaxed);

    for (u32 i = 0; i < _count; ++i) {
        if (_sinks[i] != nullptr) {
            _sinks[i]->OnEvent(eventWithSeq);
        }
    }
}

} // namespace bench
} // namespace core
