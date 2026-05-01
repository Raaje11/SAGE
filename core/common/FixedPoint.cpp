// FixedPoint.cpp
// Implements from_double(), to_double(), from_string().
// Deliberately NOT in the header — prevents these from being
// inlined into any hot-path translation unit via ThinLTO.
// These are startup and logging paths only.

#include "FixedPoint.hpp"

#include <cmath>         // std::round, std::isfinite
#include <stdexcept>     // std::invalid_argument, std::overflow_error
#include <charconv>      // std::from_chars
#include <system_error>  // std::errc
#include <limits>        // std::numeric_limits
#include <cstddef>       // std::size_t

namespace core::common {


// ── from_double ──────────────────────────────────────────────────────────────
// Called ONLY at startup (config parsing) or test setup.
// std::round avoids truncation bias at the 8th decimal place.

[[nodiscard]] FixedPrice FixedPrice::from_double(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(
            "FixedPrice::from_double: non-finite input (inf or nan)");
    }

    const double scaled = value * static_cast<double>(PRICE_SCALE);

    // Guard against int64_t overflow before cast.
    constexpr double MAX_RAW =
        static_cast<double>(std::numeric_limits<std::int64_t>::max());
    constexpr double MIN_RAW =
        static_cast<double>(std::numeric_limits<std::int64_t>::min());

    if (scaled > MAX_RAW || scaled < MIN_RAW) {
        throw std::overflow_error(
            "FixedPrice::from_double: value out of int64_t range after scaling");
    }

    return FixedPrice{ static_cast<std::int64_t>(std::round(scaled)) };
}


// ── to_double ────────────────────────────────────────────────────────────────
// Used for logging and Nexus display output only.

[[nodiscard]] double FixedPrice::to_double() const {
    return static_cast<double>(raw) / static_cast<double>(PRICE_SCALE);
}


// ── from_string ──────────────────────────────────────────────────────────────
// Parses a decimal string like "1234.56789012" into raw fixed-point.
// Uses std::from_chars — no locale, no heap allocation.
// Called only during config loading, never during trading session.

[[nodiscard]] FixedPrice FixedPrice::from_string(const char* str) {
    if (!str || *str == '\0') {
        throw std::invalid_argument("FixedPrice::from_string: null or empty input");
    }

    // Split on decimal point
    const char* dot = str;
    while (*dot && *dot != '.') ++dot;

    // Parse integer part
    std::int64_t integer_part = 0;
    auto [ptr, ec] = std::from_chars(str, dot, integer_part);
    if (ec != std::errc{}) {
        throw std::invalid_argument("FixedPrice::from_string: invalid integer part");
    }

    std::int64_t raw_value = integer_part
        * static_cast<std::int64_t>(PRICE_SCALE);

    // Parse fractional part if present
    if (*dot == '.') {
        const char* frac_start = dot + 1;
        const char* frac_end   = frac_start;
        while (*frac_end >= '0' && *frac_end <= '9') ++frac_end;

        std::size_t frac_len = static_cast<std::size_t>(frac_end - frac_start);
        if (frac_len > 0) {
            std::int64_t frac_part = 0;
            auto [fptr, fec] = std::from_chars(frac_start, frac_end, frac_part);
            if (fec != std::errc{}) {
                throw std::invalid_argument(
                    "FixedPrice::from_string: invalid fractional part");
            }

            // Scale fractional part to PRICE_SCALE digits
            // e.g. "56" with PRICE_SCALE=10^8 → 56 * 10^6 = 5_600_000_00
            constexpr std::size_t MAX_FRAC_DIGITS = 8; // log10(PRICE_SCALE)
            std::int64_t frac_scale = 1;
            for (std::size_t i = frac_len; i < MAX_FRAC_DIGITS; ++i) {
                frac_scale *= 10;
            }

            // Preserve sign of integer part for negative prices
            if (raw_value < 0 || integer_part < 0) {
                raw_value -= frac_part * frac_scale;
            } else {
                raw_value += frac_part * frac_scale;
            }
        }
    }

    return FixedPrice{ raw_value };
}


} // namespace core::common