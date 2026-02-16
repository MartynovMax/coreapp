#pragma once

// =============================================================================
// metric_value.hpp
// Typed metric value with NA (not applicable) semantics.
// =============================================================================

#include "../../core/base/core_types.hpp"

namespace core {
namespace bench {

enum class MetricValueType : u8 {
    NA,         // Not applicable / unavailable
    U64,
    I64,
    F64,
};

struct MetricValue {
    MetricValueType type;
    union {
        u64 u64Value;
        i64 i64Value;
        f64 f64Value;
    } data;

    MetricValue() noexcept : type(MetricValueType::NA), data{} {}

    static MetricValue FromU64(u64 value) noexcept {
        MetricValue v;
        v.type = MetricValueType::U64;
        v.data.u64Value = value;
        return v;
    }

    static MetricValue FromI64(i64 value) noexcept {
        MetricValue v;
        v.type = MetricValueType::I64;
        v.data.i64Value = value;
        return v;
    }

    static MetricValue FromF64(f64 value) noexcept {
        MetricValue v;
        v.type = MetricValueType::F64;
        v.data.f64Value = value;
        return v;
    }

    static MetricValue NA() noexcept {
        return MetricValue();
    }

    [[nodiscard]] bool IsNA() const noexcept { return type == MetricValueType::NA; }
    [[nodiscard]] bool IsU64() const noexcept { return type == MetricValueType::U64; }
    [[nodiscard]] bool IsI64() const noexcept { return type == MetricValueType::I64; }
    [[nodiscard]] bool IsF64() const noexcept { return type == MetricValueType::F64; }

    [[nodiscard]] u64 AsU64() const noexcept {
        return data.u64Value;
    }

    [[nodiscard]] i64 AsI64() const noexcept {
        return data.i64Value;
    }

    [[nodiscard]] f64 AsF64() const noexcept {
        return data.f64Value;
    }

    [[nodiscard]] bool TryGetU64(u64& out) const noexcept {
        if (type != MetricValueType::U64) return false;
        out = data.u64Value;
        return true;
    }

    [[nodiscard]] bool TryGetI64(i64& out) const noexcept {
        if (type != MetricValueType::I64) return false;
        out = data.i64Value;
        return true;
    }

    [[nodiscard]] bool TryGetF64(f64& out) const noexcept {
        if (type != MetricValueType::F64) return false;
        out = data.f64Value;
        return true;
    }
};

} // namespace bench
} // namespace core
