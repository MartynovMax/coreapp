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
- `--format=<mode>` - Output format: none, text, jsonl, summary, all (default: text)
- `--out=<path>` - Output file path
- `--help`, `-h` - Show help message
- `--verbose`, `-v` - Enable verbose output

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

## Important Notes on Phase Metrics and Completion

- **Bulk Reclaim (FreeAll):**
  - All live allocations are actually freed during bulk reclaim (FreeAll).
  - The freed objects and bytes are tracked separately in `reclaimFreeCount` and `reclaimBytesFreed` metrics.
  - Total deallocations are reported as `totalFreeCount = freeCount + internalFreeCount + reclaimFreeCount`.
  - Total bytes freed are reported as `totalBytesFreed = bytesFreed + internalBytesFreed + reclaimBytesFreed`.
  - The time spent in bulk reclaim (FreeAll) is included in the phase duration and performance metrics (ops/sec, throughput).

- **Time-based Completion:**
  - The main operation loop is limited by `operationCount`. If you want to use a time-based completion callback, you must set a sufficiently large `operationCount` to allow the callback to trigger. If `operationCount` is zero, the phase will terminate immediately and the callback will not be called.
  - This is by design for determinism and reproducibility. If you need unlimited or time-based phases, set `operationCount` to a large value and use a custom completion callback.

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
- If `operationCount == 0`, the phase will not create a LifetimeTracker and will only call `completionCheck` and/or `customOperation` once if provided.
- This allows for time-based or callback-only phases without dummy operations.

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

### Parameter Validation
- `allocFreeRatio` is always clamped to [0,1].
- For `AlignmentDistributionType::Typical64`, bucketCount is clamped to 4 if using defaults.
- All size/alignment distributions are clamped to valid ranges.

### API Invariants
- All operations are counted and validated. If a buffer cannot be allocated for LifetimeTracker, the phase will not track allocations and will free untracked allocations immediately.
- For Bounded/FIFO, ring buffer is used for O(1) performance. For large live-sets, this avoids O(n) shifts.

### Responsibility for Metrics
- If you override operation flow with customOperation, you are responsible for updating all phase metrics in PhaseContext.

