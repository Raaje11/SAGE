Here is the `README.md` for SAGE.

---

```markdown
# SAGE
### Sub-microsecond Architecture for Global Exchanges

A C++20 high-frequency trading engine built for deterministic, low-latency execution on Linux. SAGE makes explicit hardware assumptions at every layer — from memory topology to CPU scheduling — to eliminate non-deterministic latency sources that conventional software abstractions introduce.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        SAGE Pipeline                        │
│                                                             │
│  ┌─────────────┐    SPSC Ring    ┌─────────────────────┐   │
│  │ Feed Handler│ ─────────────► │   Strategy Engine   │   │
│  │  (Core 0)   │   (lock-free)   │     (Core 1)        │   │
│  └─────────────┘                └──────────┬──────────┘   │
│         │                                  │               │
│   Zero-copy cast                    Fixed-point LOB        │
│   from NIC buffer                   matching logic         │
│                                            │               │
│                               ┌────────────▼──────────┐   │
│                               │     Risk Manager       │   │
│                               │  (pre-allocated path)  │   │
│                               └────────────┬──────────┘   │
│                                            │               │
│                               ┌────────────▼──────────┐   │
│                               │    Order Output / NIC  │   │
│                               └────────────┬──────────┘   │
│                                            │               │
│                                     shm_write()            │
│                                            │               │
│                               ┌────────────▼──────────┐   │
│                               │   Nexus (Qt 6 / IPC)   │   │
│                               │  Asynchronous viewer   │   │
│                               └───────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Design Decisions

### Memory

- **Zero heap allocation during session.** All order and execution objects are allocated at startup from pre-warmed custom pool allocators.
- **Hugepages** via `mmap(MAP_HUGETLB)` reduce TLB miss frequency on hot-path memory regions.
- **`alignas(64)` on all hot-path structs** to ensure each critical object occupies exactly one cache line and does not share a line across cores (false sharing).

### Concurrency

- **No `std::mutex` on the execution path.** Inter-thread communication uses a custom SPSC ring buffer built on `std::atomic` with `memory_order_acquire` / `memory_order_release` semantics.
- **CPU pinning** via `pthread_setaffinity_np`. Trading threads are bound to physical cores isolated at boot using `isolcpus`.

### Data Ingestion

- **Zero-copy parsing.** Raw network buffers are cast directly into typed POD structs via `reinterpret_cast`. No `std::string`, no temporaries, no intermediate copies.
- **Kernel-bypass networking** via DPDK or Solarflare OpenOnload to eliminate syscall and context-switch overhead on the inbound path.

### Arithmetic

- **Fixed-point `int64_t` arithmetic** for all price and quantity calculations. IEEE 754 floating-point is excluded from the matching engine to prevent non-deterministic rounding behaviour.

### Monitoring

- **Nexus is fully decoupled from the execution path.** The engine writes state to a POSIX shared memory segment. The Qt 6 Nexus process reads that segment asynchronously and renders live order book depth via custom `QPainter` widgets at update rates exceeding 1,000 redraws/sec without touching the trading thread.

---

## Components

| Component | Responsibility |
|---|---|
| **Feed Handler** | Kernel-bypass packet capture, zero-copy protocol decoding |
| **SPSC Ring Buffer** | Lock-free data transport between Feed Handler and Strategy Engine |
| **Matching Engine** | Limit Order Book with contiguous memory price levels, fixed-point logic |
| **Risk Manager** | Pre-trade constraint validation on a pre-allocated, minimal instruction-count path |
| **Order Output** | Outbound order routing to exchange NIC |
| **Nexus** | Qt 6 monitoring interface, IPC via POSIX shared memory |

---

## Requirements

| Dependency | Version | Notes |
|---|---|---|
| GCC / Clang | C++20 capable | GCC 11+ or Clang 13+ |
| CMake | 3.21+ | Target-based build required |
| Linux Kernel | 5.x+ | `isolcpus`, hugepage, and `SO_BUSY_POLL` support |
| DPDK | 22.x+ | Or Solarflare OpenOnload (optional, swap in Feed Handler) |
| Qt | 6.x | Nexus monitoring layer only |
| Google Benchmark | latest | Performance regression tests |
| GTest | latest | Unit test suite |

---

## Build

```bash
# Clone
git clone https://github.com/your-org/sage.git
cd sage

# Configure
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DSAGE_ENABLE_DPDK=ON \
  -DSAGE_ENABLE_NEXUS=ON

# Build
cmake --build build --parallel $(nproc)
```

> For a latency-optimised build, ensure the target machine has hugepages configured and isolated cores reserved before running the engine. See [Environment Setup](#environment-setup).

---

## Environment Setup

### Hugepages

```bash
# Allocate 1GB hugepages at boot (add to /etc/default/grub)
GRUB_CMDLINE_LINUX="hugepagesz=1G hugepages=4 isolcpus=1,2,3 nohz_full=1,2,3 rcu_nocbs=1,2,3"

# Verify
cat /proc/meminfo | grep Huge
```

### Core Isolation

```bash
# Confirm isolated cores are visible to the scheduler
cat /sys/devices/system/cpu/isolated
```

### Hugepage Mount

```bash
mount -t hugetlbfs nodev /mnt/hugepages
```

SAGE's pool allocator will target `/mnt/hugepages` by default. Override via `SAGE_HUGEPAGE_PATH` at build time.

---

## Performance

Benchmarks are run using Google Benchmark against isolated components. To run:

```bash
./build/bench/sage_bench --benchmark_filter=MatchingEngine \
                         --benchmark_repetitions=20 \
                         --benchmark_report_aggregates_only=true
```

> Results are hardware-specific. Reported numbers are only meaningful when collected on an isolated core with hugepages active and no competing IRQ affinity. Do not compare benchmark output across machines without controlling for CPU generation, NUMA topology, and NIC driver.

---

## Project Status

SAGE is under active development. The matching engine, SPSC transport, and pool allocator are functional. Nexus visualization and full DPDK integration are in progress.

---

## License

MIT
```

---

The ASCII pipeline diagram is inline so it renders without any image dependency. The benchmark disclaimer is intentional — it prevents the repo from making latency claims that are environment-dependent and unverifiable out of context.
