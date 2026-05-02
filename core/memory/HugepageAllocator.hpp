#pragma once
// HugepageAllocator.hpp
// Allocates contiguous memory regions backed by 1GB hugepages (MAP_HUGETLB)
// on Linux. Reduces TLB miss frequency on all hot-path pool regions.
//
// On Windows: falls back to aligned_alloc with a visible warning.
// The fallback is development-only — production deployment requires Linux
// with hugepages configured via HugepageSetup.sh.
//
// RULES:
//   - allocate() returns 64-byte aligned memory or throws.
//   - warm()     must be called after every allocation before trading begins.
//   - This class is not copyable or movable — it owns a raw OS resource.

#include <cstddef>       // std::size_t
#include <stdexcept>     // std::runtime_error

#include "Platform.hpp"

namespace core::memory {


class HugepageAllocator {
public:

    // ── Construction / Destruction ───────────────────────────────────────────

    HugepageAllocator() noexcept = default;

    // Not copyable — owns a raw OS mapping
    HugepageAllocator(const HugepageAllocator&)            = delete;
    HugepageAllocator& operator=(const HugepageAllocator&) = delete;

    // Not movable — callers hold raw pointers into the mapping
    HugepageAllocator(HugepageAllocator&&)            = delete;
    HugepageAllocator& operator=(HugepageAllocator&&) = delete;

    ~HugepageAllocator() = default;


    // ── Core Interface ───────────────────────────────────────────────────────

    // Allocates `bytes` of memory.
    //
    // Linux:   mmap with MAP_HUGETLB | MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE
    //          MAP_POPULATE forces physical page mapping at allocation time —
    //          no page faults occur during the trading session.
    //
    // Windows: aligned_alloc(64, bytes) with a logged warning.
    //          Result is usable but not hugepage-backed.
    //
    // Returns: 64-byte aligned pointer. Never returns nullptr.
    // Throws:  std::runtime_error if the OS allocation fails.
    //
    // RULE: Call warm() on the returned pointer before use.
    [[nodiscard]] static void* allocate(std::size_t bytes);


    // Deallocates a region previously returned by allocate().
    //
    // Linux:   munmap
    // Windows: free (matches aligned_alloc)
    //
    // Passing a pointer not returned by allocate() is undefined behaviour.
    static void deallocate(void* ptr, std::size_t bytes) noexcept;


    // Touches every page in [ptr, ptr+bytes) by writing zero.
    //
    // Purpose: forces the kernel to map all physical pages immediately.
    //          Without this, pages are mapped lazily on first access —
    //          which means the first trading-session access to each page
    //          triggers a page fault (~1–10 us stall).
    //
    // Must be called once after allocate(), before the pool goes live.
    // Safe to call multiple times — idempotent.
    static void warm(void* ptr, std::size_t bytes) noexcept;


    // ── Diagnostics ─────────────────────────────────────────────────────────

    // Returns true if the platform supports MAP_HUGETLB.
    // Always false on Windows — use to log configuration warnings at startup.
    [[nodiscard]] static constexpr bool hugepages_supported() noexcept {
#if defined(SAGE_PLATFORM_LINUX)
        return true;
#else
        return false;
#endif
    }

    // Returns the minimum allocation granularity.
    // On Linux with 1GB hugepages: 1073741824 (1 << 30).
    // On Windows: 64 (aligned_alloc granularity used here).
    [[nodiscard]] static constexpr std::size_t page_size() noexcept {
#if defined(SAGE_PLATFORM_LINUX)
        return static_cast<std::size_t>(1) << 30; // 1 GB
#else
        return 64;
#endif
    }

};


} // namespace core::memory