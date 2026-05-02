#pragma once
// PoolAllocator.hpp
// O(1) fixed-size object pool backed by a HugepageAllocator region.
//
// Eliminates all heap allocation (new / malloc) from the trading session.
// All memory is pre-allocated and pre-warmed at startup. During the session,
// allocate() and deallocate() are stack-discipline index operations — no OS
// involvement, no heap fragmentation, no lock contention.
//
// DESIGN:
//   - Backed by a single contiguous hugepage region (one mmap call).
//   - Free slots tracked via a flat index stack (free_stack[]).
//   - allocate() pops an index  → O(1), ~2-4 instructions on hot path.
//   - deallocate() pushes index → O(1), ~2-4 instructions on hot path.
//   - Pool array is placement-constructed over the hugepage region.
//
// RULES:
//   - allocate() returns nullptr on exhaustion — never throws.
//   - deallocate() on nullptr is a no-op.
//   - deallocate() on a pointer not from this pool is undefined behaviour.
//   - Not thread-safe — each pool is owned by exactly one thread.
//   - N must be a power of 2 (enforced by static_assert).

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <cassert>

#include "Platform.hpp"
#include "HugepageAllocator.hpp"

namespace core::memory {


template<typename T, std::size_t N>
class PoolAllocator {

    // ── Compile-time invariants ──────────────────────────────────────────────

    static_assert((N & (N - 1)) == 0,
        "PoolAllocator: N must be a power of 2");

    static_assert(N >= 2,
        "PoolAllocator: N must be at least 2");

    static_assert(std::is_trivially_destructible_v<T>,
        "PoolAllocator: T must be trivially destructible. "
        "Hot-path objects must not have non-trivial destructors.");


public:

    // ── Construction / Destruction ───────────────────────────────────────────

    PoolAllocator();
    ~PoolAllocator() noexcept;

    PoolAllocator(const PoolAllocator&)            = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&)                 = delete;
    PoolAllocator& operator=(PoolAllocator&&)      = delete;


    // ── Hot Path Interface ───────────────────────────────────────────────────

    // Returns a pointer to an uninitialised T slot.
    // Returns nullptr if pool is exhausted — caller must handle via
    // SAGE_UNLIKELY branch. Never throws.
    [[nodiscard]] SAGE_FORCE_INLINE T* allocate() noexcept;

    // Returns a slot to the pool. Does NOT call T's destructor.
    // nullptr is a no-op. Non-pool pointer fires assert in debug builds.
    SAGE_FORCE_INLINE void deallocate(T* ptr) noexcept;


    // ── Diagnostics ─────────────────────────────────────────────────────────

    [[nodiscard]] SAGE_FORCE_INLINE std::size_t available() const noexcept {
        return top_;
    }

    [[nodiscard]] static constexpr std::size_t capacity() noexcept {
        return N;
    }

    [[nodiscard]] SAGE_FORCE_INLINE bool empty() const noexcept {
        return top_ == N;
    }

    [[nodiscard]] SAGE_FORCE_INLINE bool exhausted() const noexcept {
        return top_ == 0;
    }

    [[nodiscard]] static constexpr std::size_t region_bytes() noexcept {
        return N * sizeof(T);
    }


private:

    // ── Data members ─────────────────────────────────────────────────────────

    void*       pool_region_{ nullptr };
    std::size_t free_stack_[N]{};
    std::size_t top_{ 0 };


    // ── Private helpers ──────────────────────────────────────────────────────

    SAGE_FORCE_INLINE T* slot(std::size_t i) noexcept {
        return std::launder(
            reinterpret_cast<T*>(static_cast<char*>(pool_region_) + i * sizeof(T))
        );
    }

    SAGE_FORCE_INLINE const T* slot(std::size_t i) const noexcept {
        return std::launder(
            reinterpret_cast<const T*>(static_cast<const char*>(pool_region_) + i * sizeof(T))
        );
    }

    SAGE_FORCE_INLINE std::size_t index_of(const T* ptr) const noexcept {
        return static_cast<std::size_t>(
            ptr - reinterpret_cast<const T*>(pool_region_)
        );
    }

};


// ── Method definitions ────────────────────────────────────────────────────────
// In header for ThinLTO cross-TU inlining of allocate() and deallocate().


template<typename T, std::size_t N>
PoolAllocator<T, N>::PoolAllocator() {

    pool_region_ = HugepageAllocator::allocate(region_bytes());
    HugepageAllocator::warm(pool_region_, region_bytes());

    for (std::size_t i = 0; i < N; ++i) {
        free_stack_[i] = i;
    }
    top_ = N;
}


template<typename T, std::size_t N>
PoolAllocator<T, N>::~PoolAllocator() noexcept {
    HugepageAllocator::deallocate(pool_region_, region_bytes());
    pool_region_ = nullptr;
}


template<typename T, std::size_t N>
SAGE_FORCE_INLINE T* PoolAllocator<T, N>::allocate() noexcept {

    if (SAGE_UNLIKELY(top_ == 0)) {
        return nullptr;
    }

    const std::size_t idx = free_stack_[--top_];
    return slot(idx);
}


template<typename T, std::size_t N>
SAGE_FORCE_INLINE void PoolAllocator<T, N>::deallocate(T* ptr) noexcept {

    if (ptr == nullptr) return;

    const auto ptr_addr  = reinterpret_cast<std::uintptr_t>(ptr);
    const auto base_addr = reinterpret_cast<std::uintptr_t>(pool_region_);
    const auto end_addr  = base_addr + region_bytes();

    assert(ptr_addr >= base_addr && "deallocate: pointer below pool region");
    assert(ptr_addr <  end_addr  && "deallocate: pointer above pool region");
    assert(index_of(ptr) < N     && "deallocate: index out of range");

    free_stack_[top_++] = index_of(ptr);
}


} // namespace core::memory