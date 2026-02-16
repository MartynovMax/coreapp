#pragma once

// =============================================================================
// allocation_event_adapter.hpp
// Adapter to convert AllocationHook events to benchmark Event emissions.
// =============================================================================

#include "event_sink.hpp"
#include "event_types.hpp"
#include "../../core/memory/allocation_tracker.hpp"
#include "../../core/concurrency/atomic_u64.hpp"

namespace core {
namespace bench {

// ----------------------------------------------------------------------------
// AllocationEventAdapter
// ----------------------------------------------------------------------------

class AllocationEventAdapter {
public:
    AllocationEventAdapter(IEventSink* sink,
                          const char* experimentName,
                          const char* phaseName,
                          u32 repetitionId) noexcept;

    ~AllocationEventAdapter() noexcept;

    void Attach() noexcept;
    void Detach() noexcept;

private:
    static void OnAllocationHook(
        AllocationEvent event,
        const IAllocator* allocator,
        const AllocationRequest* request,
        const AllocationInfo* info,
        void* userData) noexcept;

    struct PtrEntry {
        void* ptr = nullptr;
        u64 allocationId = 0;
    };

    u64 FindAllocationId(void* ptr) noexcept;
    void StoreAllocationId(void* ptr, u64 allocId) noexcept;
    void RemoveAllocationId(void* ptr) noexcept;

    static constexpr u32 kMaxTrackedAllocations = 4096;

    IEventSink* _sink;
    const char* _experimentName;
    const char* _phaseName;
    u32 _repetitionId;
    AllocationListenerHandle _hookHandle;
    atomic_u64 _nextAllocationId;

    PtrEntry _ptrMap[kMaxTrackedAllocations];
};

} // namespace bench
} // namespace core
