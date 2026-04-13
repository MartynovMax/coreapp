# Article 1 — Experiment Matrix

Matrix of benchmark scenarios for collecting data for Article 1:
**"Allocation and Lifetime Design Space in C++ Systems"**

Source of truth: [`config/article1_matrix.json`](../config/article1_matrix.json)  
Runner usage: `coreapp_benchmarks --filter=article1/* --measurements=timer,counter --format=all --out=results/article1_run1`

---

## Workload Profiles

| Profile | `size_min` | `size_max` | `operation_count` | `max_live_objects` | `alloc_free_ratio` | Description |
|---|---|---|---|---|---|---|
| `fixed_small`   | 32  | 32  | 100 000 | 1 000 | 0.50 | Fixed 32-byte objects, steady churn |
| `variable_size` | 8   | 512 | 100 000 | 1 000 | 0.50 | Uniform [8–512] bytes              |
| `churn`         | 16  | 64  | 200 000 | 500   | 0.60 | High-churn small objects, smaller live-set |

---

## Full Matrix (31 scenarios)

`✓` — valid scenario · `✗` — excluded (see reasons below)

### malloc (baseline)

| Lifetime \ Workload | `fixed_small` | `variable_size` | `churn` |
|---|---|---|---|
| `fifo`       | ✓ | ✓ | ✓ |
| `lifo`       | ✓ | ✓ | ✓ |
| `random`     | ✓ | ✓ | ✓ |
| `long_lived` | ✓ | ✓ | ✓ |

**12 scenarios**

### monotonic_arena

| Lifetime \ Workload | `fixed_small` | `variable_size` | `churn` |
|---|---|---|---|
| `fifo`       | ✗ | ✗ | ✗ |
| `lifo`       | ✗ | ✗ | ✗ |
| `random`     | ✗ | ✗ | ✗ |
| `long_lived` | ✓ | ✓ | ✓ |

**3 scenarios**

> **Semantics for all MonotonicArena workloads (bulk-reclaim model):**  
> `BumpArena::Deallocate()` is a no-op — individual frees do not return memory.  
> All three workload profiles use **alloc-only** (ratio forced to 1.0) + `Arena::Reset()` at BulkReclaim phase.  
> - `fixed_small`: allocate fixed 32-byte blocks, accumulate, bulk reset.  
> - `variable_size`: sizes vary [8–512], no individual frees, bulk reset.  
> - `churn`: "allocation pressure with periodic bulk reset" — accumulate under pressure, reset at phase boundary.  
>
> Including FIFO/LIFO/Random lifetimes for MonotonicArena would mix semantics (lifetime model says X, but actual reclaim does Y), making cross-allocator comparison misleading.

### pool_allocator

| Lifetime \ Workload | `fixed_small` | `variable_size` | `churn` |
|---|---|---|---|
| `fifo`       | ✓ | ✗ | ✗ |
| `lifo`       | ✓ | ✗ | ✗ |
| `random`     | ✓ | ✗ | ✗ |
| `long_lived` | ✓ | ✗ | ✗ |

**4 scenarios**

> `PoolAllocator` requires a fixed block size at construction. Variable-size and churn workloads
> cannot be represented correctly without changing the allocator semantics.  
> Proxying through `SegregatedListAllocator` is not done here — it is a different allocator with
> different behavior and belongs in the `segregated_list` row.

### segregated_list

| Lifetime \ Workload | `fixed_small` | `variable_size` | `churn` |
|---|---|---|---|
| `fifo`       | ✓ | ✓ | ✓ |
| `lifo`       | ✓ | ✓ | ✓ |
| `random`     | ✓ | ✓ | ✓ |
| `long_lived` | ✓ | ✓ | ✓ |

**12 scenarios** · size classes: `[16, 32, 64, 128, 256, 512]` bytes, 2048 blocks each

---

## Totals

| Allocator | Valid scenarios |
|---|---|
| `malloc`          | 12 |
| `monotonic_arena` |  3 |
| `pool_allocator`  |  4 |
| `segregated_list` | 12 |
| **Total**         | **31** |

---

## Exclusion Rules

| Excluded combination | Reason |
|---|---|
| `monotonic_arena` × `fifo` / `lifo` / `random` | `BumpArena::Deallocate()` is a no-op; individual frees impossible; comparison misleading |
| `pool_allocator` × `variable_size` | `PoolAllocator` requires fixed block size; incompatible workload |
| `pool_allocator` × `churn` | Same as above |

---

## Allocator Notes

| Allocator | Implementation | Notes |
|---|---|---|
| `malloc` | `core::MallocAllocator` (wraps `SystemAllocator`) | Chosen as sole baseline; `SystemAllocator` not included (would duplicate baseline) |
| `monotonic_arena` | `core::BumpArena` via `ArenaAllocatorAdapter` | Bulk-reclaim semantics only; `Arena::Reset()` at BulkReclaim phase |
| `pool_allocator` | `core::PoolAllocator` | block_size = 32 (= fixed_small sizeMax); block_count = 1512 |
| `segregated_list` | `core::SegregatedListAllocator` | Size classes: 16/32/64/128/256/512 bytes, 2048 blocks each; fallback = malloc |
| `freelist_allocator` | **Not implemented** | Replaced by `segregated_list` in this matrix |

