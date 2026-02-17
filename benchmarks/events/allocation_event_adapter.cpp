// =============================================================================
// allocation_event_adapter.cpp
// Implementation of AllocationEventAdapter.
// =============================================================================

#include "allocation_event_adapter.hpp"
#include "../common/high_res_timer.hpp"
#include "../../core/base/core_assert.hpp"

namespace core {
namespace bench {

AllocationEventAdapter::AllocationEventAdapter(
    IEventSink* sink,
    const char* experimentName,
    const char* phaseName,
    u32 repetitionId) noexcept
    : _sink(sink),
      _experimentName(experimentName),
      _phaseName(phaseName),
      _repetitionId(repetitionId),
      _hookHandle(kInvalidListenerHandle),
      _nextAllocationId(1),
      _ptrMap{}
{
    // ASSERT(_sink != nullptr);
    // ASSERT(_experimentName != nullptr);
    // ASSERT(_phaseName != nullptr);
}

AllocationEventAdapter::~AllocationEventAdapter() noexcept {
    Detach();
}

u64 AllocationEventAdapter::FindAllocationId(void* ptr) noexcept {
    if (!ptr) return 0;

    for (const auto& entry : _ptrMap) {
        if (entry.ptr == ptr) {
            return entry.allocationId;
        }
    }
    return 0;
}

void AllocationEventAdapter::StoreAllocationId(void* ptr, u64 allocId) noexcept {
    if (!ptr) return;

    for (auto& entry : _ptrMap) {
        if (entry.ptr == nullptr) {
            entry.ptr = ptr;
            entry.allocationId = allocId;
            return;
        }
    }
    
    ASSERT(false && "AllocationEventAdapter: ptrMap overflow (>4096 live allocations)");
}

void AllocationEventAdapter::RemoveAllocationId(void* ptr) noexcept {
    if (!ptr) return;

    for (auto& entry : _ptrMap) {
        if (entry.ptr == ptr) {
            entry.ptr = nullptr;
            entry.allocationId = 0;
            return;
        }
    }
}

void AllocationEventAdapter::Attach() noexcept {
    if (_hookHandle != kInvalidListenerHandle) {
        return;
    }

    _hookHandle = RegisterAllocationListener(&OnAllocationHook, this);
}

void AllocationEventAdapter::Detach() noexcept {
    if (_hookHandle != kInvalidListenerHandle) {
        UnregisterAllocationListener(_hookHandle);
        _hookHandle = kInvalidListenerHandle;
    }

    for (auto& entry : _ptrMap) {
        entry.ptr = nullptr;
        entry.allocationId = 0;
    }
}

void AllocationEventAdapter::OnAllocationHook(
    AllocationEvent event,
    const IAllocator* allocator,
    const AllocationRequest* request,
    const AllocationInfo* info,
    void* userData) noexcept
{
    CORE_UNUSED(allocator);

    auto* adapter = static_cast<AllocationEventAdapter*>(userData);
    if (!adapter || !adapter->_sink) {
        return;
    }

    HighResTimer timer;

    switch (event) {
        case AllocationEvent::AllocateEnd: {
            if (!info) break;

            const bool success = (info->ptr != nullptr);
            const u64 allocId = adapter->_nextAllocationId.fetch_add(1, memory_order::relaxed);

            if (success && info->ptr) {
                adapter->StoreAllocationId(info->ptr, allocId);
            }

            Event evt{};
            evt.type = EventType::Allocation;
            evt.experimentName = adapter->_experimentName;
            evt.phaseName = adapter->_phaseName;
            evt.repetitionId = adapter->_repetitionId;
            evt.timestamp = timer.Now();
            evt.data.allocation.allocationId = allocId;
            evt.data.allocation.ptr = info->ptr;
            evt.data.allocation.size = info->size;
            evt.data.allocation.alignment = info->alignment;
            evt.data.allocation.tag = info->tag;
            evt.data.allocation.success = success;
            evt.data.allocation.opIndex = ~0ULL;
            adapter->_sink->OnEvent(evt);

            if (!success) {
                Event failEvt{};
                failEvt.type = EventType::AllocationFailed;
                failEvt.experimentName = adapter->_experimentName;
                failEvt.phaseName = adapter->_phaseName;
                failEvt.repetitionId = adapter->_repetitionId;
                failEvt.timestamp = timer.Now();
                failEvt.data.failure.reason = FailureReason::OutOfMemory;
                failEvt.data.failure.message = "Allocation returned nullptr";
                failEvt.data.failure.opIndex = ~0ULL;
                failEvt.data.failure.isRecoverable = false;
                adapter->_sink->OnEvent(failEvt);
            }
            break;
        }

        case AllocationEvent::DeallocateBegin: {
            if (!info || !info->ptr) break;

            u64 allocId = adapter->FindAllocationId(info->ptr);
            if (allocId != 0) {
                adapter->RemoveAllocationId(info->ptr);
            }

            Event evt{};
            evt.type = EventType::Free;
            evt.experimentName = adapter->_experimentName;
            evt.phaseName = adapter->_phaseName;
            evt.repetitionId = adapter->_repetitionId;
            evt.timestamp = timer.Now();
            evt.data.free.allocationId = allocId;
            evt.data.free.ptr = info->ptr;
            evt.data.free.size = info->size;
            evt.data.free.alignment = info->alignment;
            evt.data.free.tag = info->tag;
            evt.data.free.opIndex = ~0ULL;
            adapter->_sink->OnEvent(evt);
            break;
        }

        default:
            break;
    }
}

} // namespace bench
} // namespace core
