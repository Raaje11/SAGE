#pragma once
// Single inclusion; prevents macro redefinition across translation units.

// For std::size_t in prefetch and potential alignment usage.


// ============================================================
// Platform Detection
// ============================================================

#if defined(_WIN32) || defined(_WIN64)
    #define SAGE_PLATFORM_WINDOWS 1
#else
    #define SAGE_PLATFORM_LINUX 1
#endif
// Defines a single active platform macro.
// Centralizes OS branching; no OS checks outside this file.


// ============================================================
// Compiler Detection
// ============================================================

#if defined(__clang__)
    #define SAGE_COMPILER_CLANG 1
#elif defined(__GNUC__)
    #define SAGE_COMPILER_GCC 1
#else
    #define SAGE_COMPILER_UNKNOWN 1
#endif
// Distinguishes Clang/GCC for attribute/builtin support.


// ============================================================
// Cache Alignment
// ============================================================

#define SAGE_CACHE_ALIGN alignas(64)
// Expands to 64-byte alignment.
// Matches typical x86-64 cache line size for false-sharing control.


// ============================================================
// Inlining Control
// ============================================================

#if defined(SAGE_COMPILER_CLANG) || defined(SAGE_COMPILER_GCC)

    #define SAGE_FORCE_INLINE inline __attribute__((always_inline))
    // Forces inlining even under optimization boundaries.

    #define SAGE_NO_INLINE __attribute__((noinline))
    // Prevents inlining; useful for isolating hot/cold paths.

#else

    #define SAGE_FORCE_INLINE inline
    // Fallback: compiler decides.

    #define SAGE_NO_INLINE
    // No-op fallback.

#endif


// ============================================================
// Branch Prediction Hints
// ============================================================

#if defined(SAGE_COMPILER_CLANG) || defined(SAGE_COMPILER_GCC)

    #define SAGE_LIKELY(x)   __builtin_expect(!!(x), 1)
    // Hints branch predictor: condition expected true.

    #define SAGE_UNLIKELY(x) __builtin_expect(!!(x), 0)
    // Hints branch predictor: condition expected false.

#else

    #define SAGE_LIKELY(x)   (x)
    #define SAGE_UNLIKELY(x) (x)
    // Fallback: no prediction hints.

#endif


// ============================================================
// Prefetch
// ============================================================

#if defined(SAGE_COMPILER_CLANG) || defined(SAGE_COMPILER_GCC)

    #define SAGE_PREFETCH(addr) __builtin_prefetch((addr), 0, 3)
    // Prefetch address into cache:
    //   rw=0 (read), locality=3 (high temporal locality).

#else

    #define SAGE_PREFETCH(addr)
    // No-op fallback.

#endif


// ============================================================
// Linux-specific Guards (centralized)
// ============================================================

#if defined(SAGE_PLATFORM_LINUX)

    #define SAGE_LINUX_ONLY(x) x
    // Expands code only on Linux.

#else

    #define SAGE_LINUX_ONLY(x)
    // Strips Linux-only code on non-Linux platforms.

#endif


// ============================================================
// Windows-specific Guards (optional symmetry)
// ============================================================

#if defined(SAGE_PLATFORM_WINDOWS)

    #define SAGE_WINDOWS_ONLY(x) x
    // Expands code only on Windows.

#else

    #define SAGE_WINDOWS_ONLY(x)
    // Strips Windows-only code otherwise.

#endif