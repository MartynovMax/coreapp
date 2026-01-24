// =============================================================================
// tick_manager.cpp
// Implementation of TickManager for periodic tick event emission.
// =============================================================================

#include "tick_manager.hpp"
#include "../events/event_sink.hpp"
#include "../events/event_types.hpp"
#include "core/memory/memory_ops.hpp"

namespace core {
namespace bench {

TickManager::TickManager(u64 tickInterval) noexcept
    : _tickInterval(tickInterval), _lastTickOpIndex(0) {}

void TickManager::OnOperation(const TickContext& ctx) noexcept {

}

void TickManager::EmitTickEvent(const TickContext& ctx) noexcept {

}

u64 TickManager::GetTickInterval() const noexcept {
    return _tickInterval;
}

} // namespace bench
} // namespace core
