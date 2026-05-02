// HugepageAllocator.cpp
// OS-specific implementation of hugepage-backed memory allocation.
//
// Linux path:  mmap with MAP_HUGETLB | MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE
// Windows path: aligned_alloc fallback with startup warning.
//
// This file must never be included by any hot-path translation unit.
// It is called exactly once per pool at engine startup, never during
// the trading session.

#include "HugepageAllocator.hpp"

#include <cstring>    // std::memset
#include <cstdio>     // std::fprintf (stderr warning — no heap, no streams)
#include <cassert>    // assert

#if defined(SAGE_PLATFORM_LINUX)
    #include <sys/mman.h>   // mmap, munmap, MAP_HUGETLB, MAP_POPULATE
    #include <errno.h>      // errno
    #include <string.h>     // strerror (for error messages only)
#elif defined(SAGE_PLATFORM_WINDOWS)
    #include <cstdlib>      // _aligned_malloc, _aligned_free
#endif

namespace core::memory {


// ── allocate() ───────────────────────────────────────────────────────────────

[[nodiscard]] void* HugepageAllocator::allocate(std::size_t bytes) {

#if defined(SAGE_PLATFORM_LINUX)

    // MAP_HUGETLB:  use hugepage-backed virtual memory
    // MAP_ANONYMOUS: not backed by a file descriptor
    // MAP_PRIVATE:   copy-on-write; changes not shared with other processes
    // MAP_POPULATE:  pre-fault all pages — eliminates trading-session page faults
    void* ptr = ::mmap(
        nullptr,
        bytes,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_POPULATE,
        -1,
        0
    );

    if (ptr == MAP_FAILED) {
        // Capture errno before any other call can overwrite it
        const int err = errno;

        // Attempt fallback: mmap without MAP_HUGETLB
        // This happens when hugepages are not configured on the system.
        // The engine will still function but without TLB optimisation.
        // SystemInit must check hugepages_supported() and warn the operator.
        std::fprintf(stderr,
            "[HugepageAllocator] WARNING: MAP_HUGETLB failed (errno %d: %s). "
            "Falling back to standard mmap. "
            "Run HugepageSetup.sh to configure hugepages.\n",
            err, ::strerror(err));

        ptr = ::mmap(
            nullptr,
            bytes,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
            -1,
            0
        );

        if (ptr == MAP_FAILED) {
            const int err2 = errno;
            throw std::runtime_error(
                std::string("[HugepageAllocator] mmap fallback failed (errno ")
                + std::to_string(err2) + ": " + ::strerror(err2) + ")"
            );
        }
    }

    // mmap with MAP_ANONYMOUS is guaranteed page-aligned (typically 4KB or 2MB).
    // Verify 64-byte alignment invariant regardless.
    assert(reinterpret_cast<std::uintptr_t>(ptr) % 64 == 0
        && "mmap returned non-64-byte-aligned pointer");

    return ptr;

#elif defined(SAGE_PLATFORM_WINDOWS)

    // Hugepages require SeImpersonatePrivilege on Windows and are not
    // generally available in development environments.
    // Use _aligned_malloc as a functional stub.
    //
    // This path is DEVELOPMENT ONLY. Do not deploy to production on Windows.
    std::fprintf(stderr,
        "[HugepageAllocator] WARNING: Hugepages not available on Windows. "
        "Using _aligned_malloc(%zu, 64) stub. "
        "Performance characteristics will differ from Linux production.\n",
        bytes);

    void* ptr = ::_aligned_malloc(bytes, 64);

    if (ptr == nullptr) {
        throw std::runtime_error(
            "[HugepageAllocator] _aligned_malloc failed — out of memory"
        );
    }

    assert(reinterpret_cast<std::uintptr_t>(ptr) % 64 == 0
        && "_aligned_malloc returned non-64-byte-aligned pointer");

    return ptr;

#else
    #error "HugepageAllocator: unsupported platform. Add a platform guard in Platform.hpp."
#endif

}


// ── deallocate() ─────────────────────────────────────────────────────────────

void HugepageAllocator::deallocate(void* ptr, std::size_t bytes) noexcept {

    if (ptr == nullptr) return;

#if defined(SAGE_PLATFORM_LINUX)

    const int result = ::munmap(ptr, bytes);

    if (result != 0) {
        // munmap failure is non-recoverable but must not throw from noexcept.
        // Log to stderr and continue — the process is likely shutting down.
        std::fprintf(stderr,
            "[HugepageAllocator] WARNING: munmap(%p, %zu) failed (errno %d: %s)\n",
            ptr, bytes, errno, ::strerror(errno));
    }

#elif defined(SAGE_PLATFORM_WINDOWS)

    // bytes parameter intentionally unused on Windows —
    // _aligned_free does not require the size.
    (void)bytes;
    ::_aligned_free(ptr);

#endif

}


// ── warm() ───────────────────────────────────────────────────────────────────

void HugepageAllocator::warm(void* ptr, std::size_t bytes) noexcept {

    if (ptr == nullptr || bytes == 0) return;

    // std::memset writes to every byte in the region.
    // This forces the kernel to fault in and map every physical page
    // before the trading session begins.
    //
    // MAP_POPULATE on Linux already pre-faults pages at mmap time,
    // so this is belt-and-suspenders — it also initialises every byte
    // to zero, which eliminates uninitialised-memory reads from pools.
    std::memset(ptr, 0, bytes);

}


} // namespace core::memory