#pragma once
// Single inclusion per TU; prevents ODR violations.

#include <cstdint>
// Fixed-width integers for deterministic data representation.

#include <cstddef>
// size_t for memory sizes, indexing, and allocation interfaces.

namespace core::common {
// Scoped constant domain; avoids global symbol collisions.


// Hardware / cache topology
constexpr std::size_t CACHE_LINE_SIZE = 64;
// Bytes per cache line (x86-64 baseline).
// Used for alignment and false-sharing avoidance.


// Fixed-point numeric model
constexpr std::uint64_t PRICE_SCALE = 100'000'000ULL;
// 10^8 scaling for integer-based prices.
// Eliminates floating-point nondeterminism.


// Order book capacity limits
constexpr std::uint32_t MAX_LEVELS = 256;
// Max price levels per side.
// Bounds traversal and memory footprint.

constexpr std::uint32_t MAX_ORDERS = 65'536;
// Max concurrent resting orders.
// Defines pool/allocator capacity.

constexpr std::uint32_t MAX_EXECUTIONS = 65'536;
// Max execution records retained.
// Prevents unbounded growth of logs.


// SPSC ring buffer configuration
constexpr std::size_t SPSC_BUFFER_SIZE = 4096;
// Slot count; must be power of 2.
// Enables mask-based index wrap (no modulo).

static_assert((SPSC_BUFFER_SIZE & (SPSC_BUFFER_SIZE - 1)) == 0,
              "SPSC_BUFFER_SIZE must be a power of 2");
// Compile-time check for power-of-2 invariant.
// Fails build if constraint is violated.


// Shared memory identifier
constexpr const char* SHM_SEGMENT_NAME = "/sage_nexus_shm";
// POSIX shm object name (shm_open/mmap).
// Compile-time storage; no dynamic init.

} // namespace core::common
// End of constant definitions.