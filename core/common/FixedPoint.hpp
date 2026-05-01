#pragma once
// Fixed-point integer arithmetic for all price and quantity calculations.
// Eliminates IEEE 754 non-deterministic rounding from the matching engine.
// All hot-path operators are constexpr and noexcept — zero FPU involvement.

#pragma once

#include <cstdint>       // std::int64_t
#include <compare>       // std::strong_ordering — required by defaulted operator<=>
#include <type_traits>   // std::is_standard_layout_v, std::is_trivially_copyable_v

#include "Constants.hpp"
#include "Platform.hpp"

namespace core::common {


// ============================================================
// FixedPrice
//
// Represents a price as a scaled 64-bit integer.
//
//   real_value = raw / PRICE_SCALE
//   e.g. $1234.56789012 → raw = 123456789012
//
// Range: ±92,233,720,368.54775807 at 10^8 scale
// Precision: 8 decimal places
//
// RULE: from_double() and to_double() are intentionally
// defined in FixedPoint.cpp — NOT inlineable. They must
// never be called from a hot-path translation unit.
// ============================================================

struct FixedPrice {

    std::int64_t raw{0};

    // ── Construction ─────────────────────────────────────────

    constexpr FixedPrice() noexcept = default;

    constexpr explicit FixedPrice(std::int64_t raw_value) noexcept
        : raw(raw_value) {}


    // ── Arithmetic ───────────────────────────────────────────
    // All integer ALU. No FPU instructions emitted.

    SAGE_FORCE_INLINE constexpr FixedPrice
    operator+(FixedPrice rhs) const noexcept {
        return FixedPrice{ raw + rhs.raw };
    }

    SAGE_FORCE_INLINE constexpr FixedPrice
    operator-(FixedPrice rhs) const noexcept {
        return FixedPrice{ raw - rhs.raw };
    }

    SAGE_FORCE_INLINE constexpr FixedPrice
    operator*(std::int64_t scalar) const noexcept {
        return FixedPrice{ raw * scalar };
    }

    // Division by scalar — used for average price, not matching
    SAGE_FORCE_INLINE constexpr FixedPrice
    operator/(std::int64_t scalar) const noexcept {
        return FixedPrice{ raw / scalar };
    }

    // Multiply two FixedPrice values (e.g. price * qty for notional).
    // Result is descaled back by PRICE_SCALE to prevent overflow.
    // Returns raw notional in fixed-point units.
    SAGE_FORCE_INLINE constexpr std::int64_t
    notional(std::int64_t qty) const noexcept {
        // notional = (price_raw * qty) / PRICE_SCALE
        // Intermediate: int64_t can hold up to ~9.2e18.
        // Max price_raw ~9.2e18 / 1e8 = ~9.2e10 → qty headroom ~1e8 before overflow.
        // For quantities beyond 1e8, use __int128 on GCC/Clang.
        return (raw * qty) / static_cast<std::int64_t>(PRICE_SCALE);
    }


    // ── Compound assignment ──────────────────────────────────

    SAGE_FORCE_INLINE constexpr FixedPrice&
    operator+=(FixedPrice rhs) noexcept {
        raw += rhs.raw;
        return *this;
    }

    SAGE_FORCE_INLINE constexpr FixedPrice&
    operator-=(FixedPrice rhs) noexcept {
        raw -= rhs.raw;
        return *this;
    }


    // ── Comparison (C++20 spaceship) ─────────────────────────
    // Delegates entirely to int64_t comparison — single CMP instruction.

    SAGE_FORCE_INLINE constexpr auto
    operator<=>(const FixedPrice&) const noexcept = default;

    SAGE_FORCE_INLINE constexpr bool
    operator==(const FixedPrice&) const noexcept = default;


    // ── Predicates ───────────────────────────────────────────

    SAGE_FORCE_INLINE constexpr bool is_zero()     const noexcept { return raw == 0; }
    SAGE_FORCE_INLINE constexpr bool is_positive() const noexcept { return raw >  0; }
    SAGE_FORCE_INLINE constexpr bool is_negative() const noexcept { return raw <  0; }


    // ── Conversion — NOT for hot path ────────────────────────
    // Declared here, defined in FixedPoint.cpp.
    // Marked nodiscard — result must be used or the call is a mistake.

    [[nodiscard]] static FixedPrice from_double(double value);
    [[nodiscard]] double            to_double()  const;
    [[nodiscard]] static FixedPrice from_string(const char* str);

};  // struct FixedPrice


// ============================================================
// FIXED() — compile-time literal construction
//
// Usage:  constexpr FixedPrice tick = FIXED(0, 01);  // $0.01
//         constexpr FixedPrice limit = FIXED(100, 0); // $100.00
//
// first  = integer part
// second = fractional part (already scaled to 10^8)
//
// For clean compile-time prices:
//   constexpr FixedPrice p = FixedPrice{1234'56789012LL};
// is clearer than the macro for most cases.
// ============================================================

#define FIXED_RAW(integer_units) \
    ::core::common::FixedPrice{ static_cast<std::int64_t>(integer_units) }

// Converts a double literal at compile-time (consteval — only safe at
// compile time where floating-point is deterministic by the standard).
consteval FixedPrice make_fixed(double value) noexcept {
    return FixedPrice{ static_cast<std::int64_t>(value * static_cast<double>(PRICE_SCALE)) };
}

// Usage: constexpr auto price = make_fixed(1234.56);
// This is the preferred way to create compile-time constants.
// make_fixed() is consteval — it CANNOT be called at runtime.
// Any attempt to call it at runtime is a compile error.


// ============================================================
// FixedQty
//
// Quantity is an integer count — no scaling needed.
// Kept as a distinct type to prevent accidental
// price/quantity mixing at the type system level.
// ============================================================

struct FixedQty {

    std::int64_t raw{0};

    constexpr FixedQty() noexcept = default;
    constexpr explicit FixedQty(std::int64_t v) noexcept : raw(v) {}

    SAGE_FORCE_INLINE constexpr FixedQty
    operator+(FixedQty rhs) const noexcept { return FixedQty{ raw + rhs.raw }; }

    SAGE_FORCE_INLINE constexpr FixedQty
    operator-(FixedQty rhs) const noexcept { return FixedQty{ raw - rhs.raw }; }

    SAGE_FORCE_INLINE constexpr FixedQty&
    operator+=(FixedQty rhs) noexcept { raw += rhs.raw; return *this; }

    SAGE_FORCE_INLINE constexpr FixedQty&
    operator-=(FixedQty rhs) noexcept { raw -= rhs.raw; return *this; }

    SAGE_FORCE_INLINE constexpr auto
    operator<=>(const FixedQty&) const noexcept = default;

    SAGE_FORCE_INLINE constexpr bool
    operator==(const FixedQty&) const noexcept = default;

    SAGE_FORCE_INLINE constexpr bool is_zero()     const noexcept { return raw == 0; }
    SAGE_FORCE_INLINE constexpr bool is_positive() const noexcept { return raw >  0; }

};  // struct FixedQty


// ============================================================
// Type safety asserts
// ============================================================

static_assert(std::is_standard_layout_v<FixedPrice>,
    "FixedPrice must be standard layout");
static_assert(std::is_trivially_copyable_v<FixedPrice>,
    "FixedPrice must be trivially copyable");
static_assert(sizeof(FixedPrice) == 8,
    "FixedPrice must be exactly 8 bytes (one int64_t)");

static_assert(std::is_standard_layout_v<FixedQty>,
    "FixedQty must be standard layout");
static_assert(std::is_trivially_copyable_v<FixedQty>,
    "FixedQty must be trivially copyable");
static_assert(sizeof(FixedQty) == 8,
    "FixedQty must be exactly 8 bytes");


} // namespace core::common