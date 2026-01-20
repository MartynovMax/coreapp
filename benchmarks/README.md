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
