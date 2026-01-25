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
