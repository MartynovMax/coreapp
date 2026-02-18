# Timing Noise Control Protocol

**Version:** 1.0  
**Date:** 2026-02-18  
**Status:** Specification

---

## 1. Purpose and Scope

This protocol defines the standard timing measurement methodology for all benchmarks in the system. The goal is to ensure **comparability of results** across different runs, allocator implementations, and commits.

**Applies to:**
- All experiments producing timing metrics
- All measurement systems reporting duration/latency/throughput
- All output formats (text, time-series, summary)

---

## 2. Terminology

- **Measured region** — The code section whose execution time is being measured
- **Warmup iteration** — A full workload execution before measured repetitions to warm up caches, branch predictor, and allocator structures
- **Measured iteration (repetition)** — A workload execution whose timing is recorded and included in statistics
- **Valid run** — A run satisfying all protocol requirements (minimum repetitions, no isolation violations, no runtime failures)
- **Invalid run** — A run not meeting requirements; must be explicitly marked with status and reason
- **NA (Not Applicable)** — A metric that is not applicable or cannot be computed; must be distinguishable from zero

---

## 3. Warmup Policy

### 3.1 Requirements

- **Warmup iterations:** minimum 2 (default), configurable via `--warmup=<N>`
- **Identical workload:** warmup executes the same operations as measured iterations
- **Same seed:** warmup uses the same seed for determinism
- **Excluded from statistics:** warmup timing is NOT included in median/P95/etc. calculations

### 3.2 Error Handling

- If warmup iteration fails (exception, assertion, OOM) → run status = `invalid`, failure class = `warmup_failed`
- Output must include: warmup iteration number, error type

### 3.3 Rationale

- First iteration: cold start, page faults, initial allocator structure allocation
- Second iteration: caches warmed, allocator in steady state

---

## 4. Repetition Policy

### 4.1 Requirements

- **Minimum:** 5 measured repetitions (default), configurable via `--repetitions=<N>`
- **Validation:** if fewer than minimum completed → run status = `invalid`, failure class = `insufficient_repetitions`

### 4.2 Rationale

- Median requires odd count for unambiguous center value
- 5 repetitions: median = 3rd element, sufficient for basic statistics
- P95 can be reasonably approximated
- For serious regression tracking, ≥10 repetitions recommended

---

## 5. Aggregation Policy

### 5.1 Required Aggregates

Every timing output **MUST** report:

1. **Median** (`timer.repetition_median_ns`)
   - Middle element in sorted array
   - For even N: average of two middle elements
   - Robust to outliers

2. **P95** (`timer.repetition_p95_ns`)
   - 95th percentile: value at index `⌈0.95 × N⌉` in sorted array
   - Characterizes tail behavior (worst-case)

### 5.2 Recommended Aggregates

- `timer.repetition_min_ns` — minimum value (best case)
- `timer.repetition_max_ns` — maximum value (includes outliers)
- `timer.repetition_mean_ns` — arithmetic mean (useful for throughput)

### 5.3 Units and Precision

- **Unit:** nanoseconds (ns)
- **Type:** u64 for all metrics (except mean: f64)
- **Precision:** full platform timer precision, no rounding

### 5.4 NA Representation

When a metric cannot be computed:
- **JSONL:** `null` or `{"value": null, "status": "na"}`
- **CSV:** `NA` (string literal)
- **Text:** `NA` or `N/A`

**Critical:** NA must NOT be treated as 0 in analysis.

---

## 6. Optimization Barriers

### 6.1 Problem

Compilers at optimization levels `-O2`/`-O3` may:
- Eliminate "dead" allocations as unused (DCE)
- Optimize entire benchmark loop into no-op
- Hoist invariant allocations outside measured region
- Inline and constant-fold measured operations

### 6.2 Required Barriers

#### 6.2.1 Memory Touch Barrier

Every allocated memory block **MUST** be written to and read from within measured region:

```cpp
void* ptr = allocator.Allocate(size);
if (ptr != nullptr) {
    // Write barrier
    *static_cast<u8*>(ptr) = 0xFF;
    
    // Read barrier (prevents write-only elimination)
    volatile u8 dummy = *static_cast<u8*>(ptr);
    (void)dummy;
}
```

#### 6.2.2 Pointer/Value Escape Barrier

Pointers and computed values **MUST** "escape" from optimizer's scope:

```cpp
template<typename T>
inline void DoNotOptimize(const T& value) {
#if defined(_MSC_VER)
    _ReadWriteBarrier();
    (void)value;
#elif defined(__GNUC__) || defined(__clang__)
    asm volatile("" : : "r,m"(value) : "memory");
#else
    volatile const T* ptr = &value;
    (void)ptr;
#endif
}

// Usage:
DoNotOptimize(ptr);
DoNotOptimize(dummy);
```

#### 6.2.3 Loop Barrier

Benchmark loop iteration count must not be computable at compile time.

### 6.3 Verification

- Check assembly for critical paths
- Compare timing with and without barriers (should differ)
- Test with `-O3` / `/Ox`

---

## 7. Isolation Rules

### 7.1 Goal

Measured region must measure **only the allocator under test**, not:
- Infrastructure allocations (containers, strings, logging)
- System allocations (stdio, exception handling)
- Measurement system overhead

### 7.2 Requirements

#### 7.2.1 Pre-Allocation

All auxiliary structures **MUST** be allocated **before** measured region starts:

```cpp
// SETUP (before measurement)
std::vector<void*> liveSet;
liveSet.reserve(MAX_LIVE_OBJECTS);  // Pre-allocate

std::vector<size_t> sizes;
GenerateSizes(sizes, seed);  // Generate sizes

// MEASURED REGION (timing starts here)
HighResTimer timer;
u64 start = timer.Now();

for (size_t size : sizes) {
    void* ptr = allocator.Allocate(size);  // Only this is measured
    liveSet.push_back(ptr);  // No reallocation (capacity reserved)
}

u64 end = timer.Now();
```

#### 7.2.2 Prohibited Operations in Measured Region

- `std::string` concatenation/formatting
- `printf`, `snprintf`, `std::cout`
- Container growth (`push_back` without `reserve`, map insertions with rehash)
- Exception handling (`throw`/`catch`)
- Logging, file I/O, console output

#### 7.2.3 Deferred Output

All diagnostic output, logging, and formatting **MUST** occur **after** measured region:

```cpp
// Measured region
u64 duration = MeasurePhase();

// AFTER measurement: safe to log
printf("Phase completed in %llu ns\n", duration);
```

### 7.3 Validation

If allocator provides accounting hooks:
- Record allocation count before region
- Record allocation count after region
- Verify that delta = expected workload allocations

If delta doesn't match → run status = `invalid`, failure class = `isolation_violation`

---

## 8. Validity and Failure Model

### 8.1 Valid Run Criteria

A run is **Valid** if:

1. ✓ Minimum required repetitions completed
2. ✓ No isolation violations
3. ✓ No runtime failures in measured region (exceptions, crashes, assertions)
4. ✓ All optimization barriers applied
5. ✓ Warmup completed successfully (if warmup > 0)

### 8.2 Failure Classes

| Failure Class | Cause | Action |
|---|---|---|
| `warmup_failed` | Exception/crash during warmup | Report warmup iteration number and error |
| `insufficient_repetitions` | Fewer than minimum repetitions | Report expected vs. actual |
| `isolation_violation` | Unexpected allocations in measured region | Report expected vs. actual allocation count |
| `runtime_error` | Exception/crash during measured iteration | Report repetition number and error type |
| `optimization_failure` | Barrier verification failed | Report assembly/timing mismatch |
| `timeout` | Measured iteration exceeded time limit | Report timeout threshold and actual duration |
| `user_abort` | User interrupted execution | Report signal/interruption type |

### 8.3 Output Format for Invalid Runs

#### Text:
```
[INVALID] experiment_name (seed: 12345)
  Status: INVALID
  Reason: insufficient_repetitions
  Details: Expected 5, completed 3
  Metrics: NA
```

#### JSONL:
```json
{
  "schema_version": "1.0",
  "record_type": "run_summary",
  "run_id": "run_1708281234_seed12345",
  "experiment_name": "simple_alloc_fixed_16",
  "status": "invalid",
  "failure_class": "insufficient_repetitions",
  "failure_details": {
    "expected_repetitions": 5,
    "actual_repetitions": 3
  },
  "metrics": {
    "timer.repetition_median_ns": null,
    "timer.repetition_p95_ns": null
  }
}
```

#### CSV:
```csv
run_id,experiment_name,status,failure_class,median_ns,p95_ns
run_1708281234_seed12345,simple_alloc_fixed_16,invalid,insufficient_repetitions,NA,NA
```

---

## 9. Timing Instrumentation

### 9.1 Timer Requirements

- **Resolution:** ≤100 ns (ideally ≤10 ns)
- **Monotonicity:** must not go backwards (use `CLOCK_MONOTONIC` or equivalent)
- **Low overhead:** timer call overhead < 0.1% of measured duration

### 9.2 Platform APIs

- **Windows:** `QueryPerformanceCounter` / `QueryPerformanceFrequency`
- **Linux:** `clock_gettime(CLOCK_MONOTONIC_RAW)` (RAW avoids NTP adjustments)
- **macOS:** `clock_gettime(CLOCK_MONOTONIC)` or `mach_absolute_time()`

### 9.3 Timing Placement

```cpp
void RunPhases() {
    HighResTimer timer;
    
    // Setup (NOT measured)
    PreparePhase();
    
    // START measurement
    u64 start = timer.Now();
    
    // Measured region
    ExecutePhase();
    
    // END measurement
    u64 end = timer.Now();
    
    // Publish (NOT measured)
    PublishEvent(PhaseComplete{end - start});
}
```

---

## 10. Implementation Checklist

### For Experiment Authors

When implementing a new experiment, verify:

- [ ] Warmup executes identical workload to measured iterations
- [ ] Warmup is excluded from timing statistics
- [ ] At least `minRepetitions` measured iterations performed
- [ ] All auxiliary structures pre-allocated before measured region
- [ ] No logging/IO/formatting in measured region
- [ ] Memory touch barrier applied to all allocations
- [ ] Pointer escape barrier applied (DoNotOptimize)
- [ ] Failure cases report appropriate failure class
- [ ] Invalid runs produce explicit status in output

### For Measurement System Authors

When implementing a measurement system for timing:

- [ ] Median and P95 computed correctly
- [ ] Invalid runs publish NA for all metrics
- [ ] Failure class and details included in output
- [ ] Units are explicitly nanoseconds (u64)
- [ ] No premature rounding

### For Output Format Authors

When implementing an output writer:

- [ ] Invalid runs have explicit `status` field (not just missing data)
- [ ] NA values are distinguishable from zero
- [ ] Failure class and details included for invalid runs
- [ ] Schema version recorded for future compatibility

---

## 11. Compliance Mapping

| Requirement | Verified By | Location |
|---|---|---|
| Warmup excluded from statistics | `TimerMeasurementSystem` | `benchmarks/measurement/timer_system.cpp` |
| Minimum repetitions enforced | `ExperimentRunner` | `benchmarks/runner/experiment_runner.cpp` |
| Median + P95 reported | `TimerMeasurementSystem::CalculateStatistics` | `benchmarks/measurement/timer_system.cpp` |
| NA for invalid runs | `MetricCollector` | `benchmarks/measurement/metric_collector.cpp` |
| Isolation: pre-allocation | Experiments | `benchmarks/experiments/*.cpp` (review) |
| Memory touch barrier | Workload | `benchmarks/workload/phase_executor.cpp` |
| Pointer escape barrier | Workload | `benchmarks/workload/phase_executor.cpp` |
| Failure class reporting | `OutputManager` | `benchmarks/output/output_manager.cpp` |

---

## 12. Definition of Done

Protocol is considered fully implemented when:

1. ✅ All timing outputs (text, JSONL, CSV) conform to aggregation policy
2. ✅ All experiments satisfy isolation rules (verified by review + tests)
3. ✅ All experiments apply optimization barriers (verified by assembly inspection)
4. ✅ Invalid runs are explicitly marked with failure class in all outputs
5. ✅ Regression comparison tooling correctly handles NA values and invalid runs

---

## 13. Out of Scope

The following topics are **explicitly out of scope** for this protocol (may be addressed in the future):

- CPU pinning policies
- Hardware performance counters (PMU, PAPI)
- Frequency scaling control
- NUMA-aware placement
- Container/virtualization noise mitigation

---

**Version:** 1.0  
**Date:** 2026-02-18  
**Authority:** This document is the single source of truth for timing measurement methodology.
