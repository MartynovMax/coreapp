# Benchmarks

Experiment runner for memory subsystem benchmarks.

## Build

```bash
cmake -B build -DBUILD_BENCHMARKS=ON
cmake --build build
```

## Usage

List available experiments:
```bash
./build/coreapp_benchmarks --list
```

Run all experiments:
```bash
./build/coreapp_benchmarks
```

Run with options:
```bash
./build/coreapp_benchmarks --filter=arena/* --seed=42 --warmup=3 --repetitions=10
```

## CLI Flags

- `--list` - List all available experiments
- `--filter=<pattern>` - Run experiments matching pattern (wildcards: *, ?)
- `--seed=<u64>` - Set deterministic seed (default: 0)
- `--warmup=<n>` - Number of warmup iterations (default: 0)
- `--repetitions=<n>` - Number of measured repetitions (default: 1)
- `--format=<mode>` - Output format (default: text)
  - `none` - Silent (no output)
  - `text` - Human-readable console output
  - `jsonl` - Time-series JSONL event log
  - `summary` - CSV summary with aggregated metrics
  - `all` - All formats (text + jsonl + summary)
- `--out=<path>` - Output file base path (creates `<path>.jsonl` and/or `<path>.csv`)
- `--measurements=<list>` - Comma-separated measurement systems (e.g., `timer,counter,snapshot`)
- `--help`, `-h` - Show help message
- `--verbose`, `-v` - Enable verbose output

### Structured Outputs

For machine-readable outputs suitable for analysis and archival:

```bash
# Time-series event log (JSONL)
./coreapp_benchmarks --format=jsonl --out=results/run1 --measurements=timer,counter

# Summary metrics (CSV)
./coreapp_benchmarks --format=summary --out=results/run1 --measurements=timer,counter

# Both outputs
./coreapp_benchmarks --format=all --out=results/run1 --measurements=timer,counter
```

See [`output/README.md`](output/README.md) for detailed schema documentation.

## Offline Analysis Tool

The `analysis/` directory contains a Python package for offline ingestion,
validation, reporting, comparison, and plotting of benchmark outputs.

### Quick start

```bash
# From the benchmarks/ directory:

# Report for a single run
python -m analysis report --input results/run1

# Compare two runs
python -m analysis compare --baseline results/run1 --candidate results/run2

# Latency bar chart
python -m analysis plot --input results/run1 --kind latency --output latency.png

# All commands support optional filters:
python -m analysis report --input results/run1 --allocator tlsf --experiment exp_A
```

### Running the analysis tests

```bash
# From the benchmarks/ directory:
python -m pytest analysis/tests/ -v
```

See [`analysis/README.md`](analysis/README.md) for full documentation.

## Exit Codes

- `0` - Success (all experiments passed)
- `1` - Failure (all experiments failed)
- `2` - No experiments matched filter
- `3` - Invalid CLI arguments
- `4` - Partial failure (some experiments failed)

## Creating Experiments

Implement `IExperiment` interface:

```cpp
class MyExperiment : public IExperiment {
public:
    void Setup(const ExperimentParams& params) noexcept override {
        // Initialize with params.seed
    }
    
    void Warmup() noexcept override {
        // Warmup iteration
    }
    
    void RunPhases() noexcept override {
        // Measured workload
    }
    
    void Teardown() noexcept override {
        // Cleanup
    }
    
    const char* Name() const noexcept override { return "my_experiment"; }
    const char* Category() const noexcept override { return "test"; }
    const char* Description() const noexcept override { return "Description"; }
    const char* AllocatorName() const noexcept override { return "allocator"; }
    
    static IExperiment* Create() noexcept { return new MyExperiment(); }
};
```

Register in `main_benchmarks.cpp`:

```cpp
ExperimentDescriptor desc;
desc.name = "my_experiment";
desc.category = "test";
desc.allocatorName = "allocator";
desc.description = "Description";
desc.factory = &MyExperiment::Create;
registry.Register(desc);
```

# Workload Model Overview

This section describes the workload modeling system used in the Axiom coreapp benchmarks. The system is designed for deterministic, reproducible, and extensible benchmarking of memory allocators and related subsystems.

## Key Concepts

### 1. WorkloadParams
Defines all parameters for a benchmark phase, including:
- Size distribution (Uniform, PowerOfTwo, Normal, LogNormal, Exponential, Pareto, SmallBiased, LargeBiased, Bimodal, WebServerAlloc, GameEngine, DatabaseCache, CustomBuckets)
- Alignment distribution (Fixed, PowerOfTwoRange, MatchSizePow2, Typical64, CustomBuckets)
- Allocation metadata (tag, flags)
- Operation mix (alloc/free ratio)
- Lifetime model (Fifo, Lifo, Random, Bounded, LongLived)
- Max live objects (for bounded models)
- Tick interval (for periodic events)

### 2. OperationStream
Generates a deterministic sequence of allocation/free operations based on WorkloadParams and a seeded RNG. Each operation carries size, alignment, tag, and flags, ensuring full reproducibility and flexibility.

### 3. LifetimeTracker
Tracks all live allocations and implements various lifetime models (Fifo, Lifo, Random, Bounded, LongLived). Ensures that deallocation operations are consistent with the allocation attributes and supports efficient selection/removal.

### 4. PhaseDescriptor & PhaseContext
- **PhaseDescriptor**: Describes a single phase of a benchmark, including its name, type, parameters, and callbacks for custom operations and completion checks.
- **PhaseContext**: Provides runtime context for a phase, including pointers to the allocator, RNG, event sink, and runtime metrics.

### 5. PhaseExecutor
Executes a benchmark phase using OperationStream and LifetimeTracker. Handles the main operation loop, event emission, tick management, and phase completion logic.

### 6. TickManager
Manages periodic tick events for progress reporting and metrics collection during phase execution.
- **Tick Trigger**: Ticks are triggered based on 1-based operation indexing: `(opIndex + 1) % tickInterval == 0`.
- **Tick Payload**: `TickPayload::opIndex` is 0-based (the actual operation index).
- Example: With `tickInterval = 1000`, ticks fire at operations 1000, 2000, 3000, etc., with `opIndex` values 999, 1999, 2999 in the payload.

## Extensibility
- All distributions and models are extensible via enums and parameter structs.
- Callbacks allow custom operation and completion logic per phase.
- No STL is used in public headers for maximum portability and minimal dependencies.

## Determinism
- All random decisions (size, alignment, operation type, lifetime) are made via a seeded RNG, ensuring reproducible results for the same seed and parameters.

## Example Usage
- Define a set of WorkloadParams for your scenario.
- Create a PhaseDescriptor for each phase (e.g., RampUp, Steady, BulkReclaim).
- Use PhaseExecutor to run each phase, collecting events and metrics.

See the code in `workload/` for detailed API and implementation.

## Preset Distributions

The workload model provides a set of ready-to-use preset distributions for both allocation size and alignment. These can be used directly in your experiments via the `SizePresets` and `AlignmentPresets` namespaces.

### SizePresets
- `SmallObjects()` — Uniform [8, 64] bytes
- `MediumObjects()` — Uniform [64, 512] bytes
- `LargeObjects()` — Uniform [512, 4096] bytes
- `WebServer()` — LogNormal with parameters in log-space (μ, σ), [16, 4096]. For correct usage with linear-space parameters, set `meanInLogSpace = false` and provide mean/stddev in bytes.
- `GameEntityPool()` — Bimodal: 85% [16,64], 15% [1024,2048], [8,8192]
- `DatabasePages()` — PowerOfTwo [4096, 16384]

### AlignmentPresets
- `Default()` — Fixed alignment = 0 (allocator default)
- `CacheLine()` — Fixed alignment = cache line size
- `MatchSizePow2(minA, maxA)` — Alignment = next_pow2(size), clamped
- `PowerOfTwoRange(minA, maxA)` — Random power-of-two in range

Example usage:
```cpp
params.sizeDistribution = core::bench::SizePresets::WebServer();
params.alignmentDistribution = core::bench::AlignmentPresets::CacheLine();
```

## Usage Example: Custom Workload

```cpp
core::bench::WorkloadParams params;
params.seed = 42;
params.operationCount = 100000;
params.sizeDistribution = core::bench::SizePresets::GameEntityPool();
params.alignmentDistribution = core::bench::AlignmentPresets::MatchSizePow2(8, 64);
params.lifetimeModel = core::bench::LifetimeModel::Bounded;
params.maxLiveObjects = 1000;
params.allocFreeRatio = 0.6f;

core::bench::PhaseDescriptor desc;
desc.name = "Steady";
desc.type = core::bench::PhaseType::Steady;
desc.params = params;

// ... setup PhaseContext, PhaseExecutor, etc.
```

See `workload/workload_params.hpp` for all available presets and parameter options.

## Event Model

The benchmark system provides a comprehensive event tracing mechanism for capturing experiment execution, allocation/deallocation operations, and failure conditions. All events flow through a unified **Event + EventSink** architecture.

### Event Channels

The event model supports two complementary channels:

1. **Workload-level events**: Lifecycle and metrics
   - `ExperimentBegin/End` - Experiment boundaries
   - `PhaseBegin/End/Complete` - Phase lifecycle with full metrics
   - `Tick` - Periodic progress checkpoints during phase execution

2. **Allocator-level events**: Detailed allocation tracking (opt-in via `AllocationEventAdapter`)
   - `Allocation` - Every allocation (success or failure) with correlation ID
   - `Free` - Every deallocation with correlation ID
   - `AllocationFailed` / `OutOfMemory` - Failure signals
   - `PhaseFailure` - Fatal phase errors

### Event Ordering

All events are assigned a global **eventSeqNo** (sequence number) for deterministic ordering:
- Monotonically increasing atomic counter
- Guarantees stable order even when timestamps are identical
- Enables deterministic replay and offline analysis

### Usage Example: Custom EventSink

```cpp
class MyEventSink : public IEventSink {
public:
    void OnEvent(const Event& event) noexcept override {
        switch (event.type) {
            case EventType::PhaseComplete:
                printf("Phase '%s' completed: %llu allocs, %llu frees\n",
                       event.phaseName,
                       event.data.phaseComplete.allocCount,
                       event.data.phaseComplete.freeCount);
                break;
            
            case EventType::Allocation:
                printf("Alloc [%llu]: ptr=%p, size=%llu\n",
                       event.data.allocation.allocationId,
                       event.data.allocation.ptr,
                       event.data.allocation.size);
                break;
            
            case EventType::Free:
                printf("Free [%llu]: ptr=%p\n",
                       event.data.free.allocationId,
                       event.data.free.ptr);
                break;
            
            // ... handle other event types ...
        }
    }
};

// Attach to PhaseExecutor
MyEventSink sink;
PhaseExecutor executor(desc, ctx, &sink);
executor.Execute(); // Allocation tracking enabled automatically
```

### Allocation Correlation

The `AllocationEventAdapter` automatically tracks allocation ↔ free pairs via `allocationId`:
- Each allocation gets a unique per-phase ID
- Free events carry the matching `allocationId` for correlation
- Enables memory leak detection and lifetime analysis

### Event Types

The system defines 13 event types:

| Event Type | When Emitted | Payload |
|------------|--------------|---------|
| `ExperimentBegin` | Start of experiment | None |
| `ExperimentEnd` | End of experiment | None |
| `PhaseBegin` | Start of phase | None |
| `PhaseEnd` | End of phase (after reclaim) | None |
| `PhaseComplete` | Phase completion | Full metrics summary |
| `Tick` | Periodic checkpoint | Snapshot metrics |
| `Allocation` | Every allocation | size, alignment, ptr, allocationId, success |
| `Free` | Every deallocation | ptr, size, allocationId |
| `AllocatorReset` | Bulk allocator reset | allocatorName, freedCount, freedBytes |
| `AllocatorRewind` | Allocator rewind | allocatorName, freedCount, freedBytes |
| `OutOfMemory` | OOM condition | reason, message |
| `AllocationFailed` | Allocation returned nullptr | reason, message |
| `PhaseFailure` | Fatal phase error | reason, message, opIndex |

### Event Structure

All events contain:
- `EventType type` - Event type identifier
- `const char* experimentName` - Experiment name (or "N/A")
- `const char* phaseName` - Phase name (or "N/A")
- `u32 repetitionId` - Repetition counter
- `u64 timestamp` - Nanosecond timestamp from `HighResTimer`
- `u64 eventSeqNo` - Global sequence number (assigned by `EventBus`)
- `union data` - Type-specific payload

### Allocation Tracking

Allocation/Free events require `CORE_MEMORY_TRACKING=1`:
- **Enabled**: Debug builds (default)
- **Disabled**: Release builds (no allocation events emitted)
- **Override**: Define `CORE_MEMORY_TRACKING=1` in build config

### Performance

- **Zero overhead** when `EventSink == nullptr` (no event tracking)
- **Opt-in**: Allocation tracking only enabled when sink is attached
- **Per-event cost**: ~5-10ns for sequence number assignment
- **Per-allocation cost**: ~10-50ns for correlation ID generation

## Important Notes on Phase Metrics and Completion

- **Bulk Reclaim (FreeAll):**
  - All live allocations are actually freed during bulk reclaim (FreeAll).
  - The freed objects and bytes are tracked separately in `reclaimFreeCount` and `reclaimBytesFreed` metrics.
  - Total deallocations are reported as `totalFreeCount = freeCount + internalFreeCount + reclaimFreeCount`.
  - Total bytes freed are reported as `totalBytesFreed = bytesFreed + internalBytesFreed + reclaimBytesFreed`.
  - The time spent in bulk reclaim (FreeAll) is included in the phase duration and performance metrics (ops/sec, throughput).

- **operationCount == 0 (Do-While Semantics):**
  - When `operationCount == 0`, the phase executes in loop-until-complete mode (do-while).
  - **REQUIRED**: Both `customOperation` and `completionCheck` callbacks MUST be provided.
  - **Guarantee**: At least 1 iteration is executed, even if `completionCheck` returns true immediately.
  - **Safety**: Maximum iterations (default: 10M) prevents infinite loops.
  - Use case: Time-based phases, memory-threshold phases, or custom termination logic.

- **Time-based Completion:**
  - For deterministic phases with `operationCount > 0`, the operation loop will execute exactly that many operations.
  - Time-based completion can be achieved via `completionCheck` callback, but the phase will never exceed `operationCount`.
  - For truly time-driven phases, use `operationCount = 0` with custom callbacks.

## Workload Model and Phase Semantics

### Lifetime Models and Tracking
- **FIFO/Bounded**: Now implemented as O(1) ring buffer (head/tail). RemoveIndex for idx==0 just advances head, no array copy. For other models, swap-remove is used.
- **Bounded**: When the buffer is full, the oldest allocation is forcibly freed (forcedFree). Free operations before reaching capacity are ignored. This matches a true bounded live-set.
- **Peaks**: Peak live count/bytes are reset on Clear()/FreeAll(), so shared trackers between phases do not leak peak stats.

### ReclaimMode and LifetimeTracker Requirements
- **ReclaimMode::None**: No automatic reclaim. Typically used with `externalLifetimeTracker` for multi-phase experiments.
- **ReclaimMode::FreeAll**: Automatically frees all tracked allocations at phase end. **Requires LifetimeTracker**.
- **ReclaimMode::Custom**: Uses a custom `reclaimCallback` for phase cleanup. LifetimeTracker is optional - can be used via external tracker, passed through userData, or not needed at all depending on the cleanup strategy.

### Phases with Zero Operations
- If `operationCount == 0`, the phase enters **loop-until-complete (do-while) mode**.
- **Contract**: Both `customOperation` and `completionCheck` MUST be provided, or the phase will fail with FATAL.
- **Guarantee**: Minimum 1 iteration, even if `completionCheck` immediately returns true.
- Useful for callback-driven or time-based phases without predetermined operation counts.

### callbackRng Lifetime and Usage
- `PhaseContext::callbackRng` is provided for use within callbacks (`customOperation`, `reclaimCallback`, `completionCheck`).
- **CRITICAL**: `callbackRng` is execution-scoped and MUST NOT be stored beyond the callback lifetime.
- **Workload Determinism**: OperationStream owns its own RNG (seeded from `WorkloadParams::seed`). Using `callbackRng` for workload generation breaks reproducibility.
- Use `callbackRng` only for callback-specific randomness (e.g., selecting which object to free in custom reclaim).

### Distribution Behavior
- **Exponential distribution**: Values are clamped to `[minSize, maxSize]`, not scaled. This may result in a truncated distribution.
- **LogNormal distribution**: If `meanInLogSpace == false`, parameters are treated as linear-space and converted internally.
- **allocFreeRatio**: Always normalized to [0,1]. NaN and negative values are treated as 0.0.
- **minSize/maxSize normalization**: After all swaps and adjustments, `minSize` is guaranteed to be >= 1 in both debug and release builds to prevent zero-size allocations.
- **Alignment buckets** (Typical64, CustomBuckets): Non-power-of-2 values are automatically rounded up to the next power-of-2 at construction.
  - **CustomBuckets**: If `bucketCount > 16`, only the first 16 buckets are used, with weights renormalized to sum to 1.0.
  - **CustomBuckets fallback**: If buckets/weights are null, count is 0, or weightSum <= 0, the distribution falls back to `Fixed` with `CORE_DEFAULT_ALIGNMENT`.

### customOperation Semantics
- The `customOperation` callback receives `PhaseContext&` and `const Operation&`.
- **CRITICAL**: When using `customOperation`, you are **fully responsible** for updating ALL relevant metrics in `PhaseContext`:
  - `ctx.allocCount` - increment for each allocation
  - `ctx.freeCount` - increment for each deallocation
  - `ctx.bytesAllocated` - add allocated bytes
  - `ctx.bytesFreed` - add freed bytes
  - Update the `LifetimeTracker` if using one
- If metrics are not updated, phase statistics will be incomplete and misleading.
- The standard operation path (`Alloc`/`Free`) automatically updates these metrics. Custom operations bypass this automation.

### Parameter Validation and Normalization
- **allocFreeRatio**: Normalized to [0,1] at construction. NaN → 0.0, negative → 0.0, >1.0 → 1.0.
- **Size ranges**: If `minSize > maxSize`, they are swapped. If `minSize == 0`, it's set to 1.
- **Alignment ranges**: If `minAlignment > maxAlignment`, they are swapped.
- **Peak ranges** (Bimodal, GameEngine): Automatically clamped to `[minSize, maxSize]`.
- **Alignment buckets** (CustomBuckets, Typical64): Non-power-of-2 values → next power-of-2. Zero → default alignment.
- All normalizations happen in `OperationStream` constructor for consistent, release-safe behavior.

### API Invariants
- **Determinism**: Same `WorkloadParams` (including `seed`) → identical operation stream.
- **Strict Intensity**: `operationCount > 0` → exactly `operationCount` operations issued (including forced/noop).
- **Byte Counter Safety**: All byte counters use saturating addition (clamp to UINT64_MAX on overflow).
- **Tracker Contract**: At most ONE of `liveSetTracker` or `externalLifetimeTracker` can be set in `PhaseContext`.
- **Buffer Allocation**: If LifetimeTracker buffer allocation fails, the phase will FATAL (not silently degrade).

### Responsibility for Metrics
- If you override operation flow with customOperation, you are responsible for updating all phase metrics in PhaseContext.

