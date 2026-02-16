# Measurement Systems

Composable, plugin-based measurement infrastructure for benchmark experiments.

## Overview

Measurement systems decouple **what to measure** from **how workloads execute**. This enables:

- Adding/removing metrics without changing workload code
- Per-run configuration of measurement sets
- Schema-stable output with NA semantics for unavailable metrics
- Independent evolution of measurement and workload logic

## Architecture

### Core Components

1. **IMeasurementSystem**: Base interface for measurement plugins
2. **MetricCollector**: Central registry for metric publication
3. **MetricDescriptor**: Metadata (name, unit, classification, capability)
4. **MetricValue**: Typed value with NA (not applicable) support

### Measurement Systems

#### TimerMeasurementSystem
- **Purpose**: Phase and repetition duration tracking
- **Metrics**: 
  - `timer.phase_duration_ns` (exact)
  - `timer.repetition_min/max/mean/median/p95_ns` (exact)
- **Classification**: All exact (timing always available)
- **Dependencies**: None

#### CounterMeasurementSystem
- **Purpose**: Operation and byte counters with live-set tracking
- **Metrics**:
  - `counter.alloc_ops`, `counter.free_ops` (exact)
  - `counter.bytes_allocated`, `counter.bytes_freed` (exact)
  - `counter.peak_live_count`, `counter.peak_live_bytes` (optional)
  - `counter.final_live_count`, `counter.final_live_bytes` (exact)
- **Classification**: Mixed (exact + optional)
- **Dependencies**: Peak metrics require `live_set_tracking` capability

#### SnapshotMeasurementSystem
- **Purpose**: Tick-aligned periodic metric captures
- **Metrics**:
  - `snapshot.tick_N.op_index` (exact)
  - `snapshot.tick_N.alloc_count`, `snapshot.tick_N.free_count` (exact)
  - `snapshot.tick_N.live_count`, `snapshot.tick_N.live_bytes` (optional)
- **Classification**: Mixed (exact + optional)
- **Configuration**: `every_n_ticks` parameter (default: 1)
- **Limits**: Maximum 100 snapshots per run (increase `kMaxSnapshots` if needed)

## Usage

### Programmatic API

```cpp
ExperimentRunner runner(registry);

// Register measurement systems
TimerMeasurementSystem timer;
CounterMeasurementSystem counter;
runner.RegisterMeasurementSystem(&timer);
runner.RegisterMeasurementSystem(&counter);

// Run experiments
RunConfig config;
config.measuredRepetitions = 10;
runner.Run(config);

// Access metrics
MetricCollector* collector = runner.GetMetricCollector();
const MetricEntry* phaseDuration = collector->FindMetric("timer.phase_duration_ns");
if (phaseDuration && !phaseDuration->value.IsNA()) {
    printf("Phase duration: %llu ns\n", phaseDuration->value.AsU64());
}
```

### CLI Usage

```bash
# Enable specific measurement systems
./coreapp_benchmarks --measurements=timer,counter --repetitions=10

# Timer only
./coreapp_benchmarks --measurements=timer

# All systems
./coreapp_benchmarks --measurements=timer,counter,snapshot
```

## Metric Classification

### Exact
- Metric is **guaranteed accurate** when available
- Example: `timer.phase_duration_ns` (measured by hardware timer)
- Always prefer exact metrics for performance analysis

### Optional
- Metric **depends on runtime capability**
- Value is **NA** when capability unavailable
- Example: `counter.peak_live_count` requires `live_set_tracking`
- Check classification before interpreting as "0"

### Proxy
- Metric is an **approximation/estimate**
- Explicitly marked to prevent misinterpretation
- Example: hypothetical `counter.estimated_memory_overhead` (not exact)

## NA Semantics

### When NA is emitted

1. **Capability missing**: Optional metric requires feature not available
   - Example: `peak_live_count = NA` when tracking disabled
2. **System disabled**: Measurement system opted out via config
   - All metrics from disabled system = NA
3. **No data**: No events triggered metric computation
   - Example: `repetition_min_ns = NA` when repetitions = 0

### NA vs Zero

| Scenario | Correct Value | Incorrect Value |
|----------|---------------|-----------------|
| Reset-only allocator (no explicit frees) | `free_ops = NA` | `free_ops = 0` |
| Live-set tracking disabled | `peak_live_count = NA` | `peak_live_count = 0` |
| No ticks configured | `snapshot.tick_1.* = (absent)` | `snapshot.tick_1.* = 0` |

### Output Representation

- **JSON**: `null`
- **Text**: `"NA"` or `"n/a"`
- **CSV**: Empty cell with schema annotation

## Schema Stability

Measurement systems follow **schema-stable output**: all known metrics appear in output, even if value is NA.

### Benefits

- Parsers don't break on missing fields
- Easy to detect unavailable capabilities
- Consistent key ordering across runs

### Implementation

```cpp
// Disabled system still registers metrics with NA
timer.SetEnabled(false);
timer.PublishMetrics(collector);

// All timer.* keys present in output with NA values
```

## Lifecycle Hooks

Measurement systems receive events via `IEventSink::OnEvent()`:

1. **OnRunStart**: Experiment begins, reset state
2. **PhaseBegin/PhaseComplete/PhaseEnd**: Track phase boundaries
3. **Tick**: Periodic snapshots
4. **OnRunEnd**: Finalize computation
5. **PublishMetrics**: Push metrics to collector

### Event Flow

```
OnRunStart(experimentName, seed, repetitions)
  ↓
[Experiment loop: receives PhaseBegin/PhaseComplete/Tick/etc via OnEvent()]
  ↓
OnRunEnd()
  ↓
PublishMetrics(collector)
```

## Extensibility

### Adding a Custom Measurement System

```cpp
class MyCustomSystem : public IMeasurementSystem {
public:
    const char* Name() const noexcept override { return "custom"; }
    const char* Description() const noexcept override { return "My custom metrics"; }
    
    void OnRunStart(const char* experimentName, u64 seed, u32 repetitions) noexcept override {
        // Reset state
    }
    
    void OnEvent(const Event& event) noexcept override {
        if (!_enabled) return;
        
        // Filter and process relevant events
        if (event.type == EventType::PhaseComplete) {
            // Extract data
        }
    }
    
    void GetCapabilities(const MetricDescriptor** outDescriptors, u32& outCount) const noexcept override {
        *outDescriptors = _descriptors;
        outCount = _descriptorCount;
    }
    
    void PublishMetrics(MetricCollector& collector) noexcept override {
        if (!_enabled) {
            // Register NA for all metrics
            return;
        }
        
        collector.Publish("custom.my_metric", MetricValue::FromU64(value));
    }
    
private:
    static const MetricDescriptor _descriptors[];
    static constexpr u32 _descriptorCount = 1;
};
```

## Design Rationale

### Why plugin architecture?
- **Separation of concerns**: Workload = business logic, Measurement = instrumentation
- **Composability**: Mix-and-match systems per run
- **Testability**: Systems testable in isolation

### Why NA instead of zero?
- **Semantic correctness**: "not measured" ≠ "measured as zero"
- **Capability detection**: Easy to distinguish missing feature from actual zero
- **Error prevention**: Forces explicit handling of unavailable metrics

### Why schema-stable output?
- **Parser reliability**: No schema evolution on capability changes
- **Diffing**: Easy to compare runs with different enabled systems
- **Introspection**: Output self-documents available vs unavailable metrics

## Future Work

- [ ] Hardware performance counters (CPU cycles, cache misses)
- [ ] Memory allocation flamegraphs (stack traces)
- [ ] Real-time metric streaming (live dashboard)
- [ ] Distributed tracing integration (OpenTelemetry)
- [ ] Percentile histograms (HDR Histogram)

## Testing

See `benchmarks/tests/test_measurement_systems.cpp`:

- Metric value NA semantics
- Collector registration and overwrite
- Timer/Counter/Snapshot system smoke tests
- Same workload with different measurement sets
- Schema stability with disabled systems

Run tests:
```bash
cd build
ctest -R benchmark_tests -V
```

## Version

- **API Version**: 1.0
- **Date**: 2026-02-16
- **Event Compatibility**: Requires stable `EventType` and `Event` structures from `events/event_types.hpp`
