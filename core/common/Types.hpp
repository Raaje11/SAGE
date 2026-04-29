#pragma once
// POD wire types for cross-component communication (C++20).

#include <cstdint>
#include <type_traits>

#include "Constants.hpp"
#include "Platform.hpp"

namespace core::common {


// ============================================================
// ENUMS — fixed underlying size (1 byte)
// ============================================================

enum class Side : std::uint8_t {
    BID = 0,
    ASK = 1
};

enum class OrderType : std::uint8_t {
    LIMIT  = 0,
    MARKET = 1,
    CANCEL = 2
};


// ============================================================
// MarketData — cache-line isolated
// ============================================================

struct SAGE_CACHE_ALIGN MarketData {
    std::uint64_t timestamp_ns; // 8
    std::uint32_t symbol;       // 4
    std::int64_t  bid;          // 8
    std::int64_t  ask;          // 8
    std::int64_t  bid_qty;      // 8
    std::int64_t  ask_qty;      // 8
    std::uint32_t _pad;         // 4 → payload = 48
};

// Payload = 48, alignment = 64 → actual size = 64
static_assert(sizeof(MarketData) == 64, "MarketData must occupy exactly one cache line");
static_assert(alignof(MarketData) == CACHE_LINE_SIZE, "MarketData must be cache-line aligned");


// ============================================================
// Order — compact (32 bytes exact)
// ============================================================

struct Order {
    std::uint64_t order_id; // 8
    std::int64_t  price;    // 8
    std::int64_t  qty;      // 8
    std::uint32_t symbol;   // 4
    std::uint8_t  side;     // 1
    std::uint8_t  type;     // 1
    std::uint8_t  pad[2];   // 2 → total = 32
};

static_assert(sizeof(Order) == 32, "Order must be exactly 32 bytes");


// ============================================================
// Execution — immutable event (40 bytes)
// ============================================================

struct Execution {
    std::uint64_t exec_id;      // 8
    std::uint64_t order_id;     // 8
    std::int64_t  price;        // 8
    std::int64_t  qty;          // 8
    std::uint64_t timestamp_ns; // 8
};

static_assert(sizeof(Execution) == 40, "Execution must be exactly 40 bytes");


// ============================================================
// OrderRequest — routing envelope (40 bytes)
// ============================================================

struct OrderRequest {
    Order          order;     // 32
    std::uint32_t  route_id;  // 4
    std::uint32_t  flags;     // 4
};

static_assert(sizeof(OrderRequest) == 40, "OrderRequest must be exactly 40 bytes");


// ============================================================
// POD / ABI invariants (C++20)
// ============================================================

static_assert(std::is_standard_layout_v<MarketData>, "MarketData must be standard layout");
static_assert(std::is_trivially_copyable_v<MarketData>, "MarketData must be trivially copyable");

static_assert(std::is_standard_layout_v<Order>, "Order must be standard layout");
static_assert(std::is_trivially_copyable_v<Order>, "Order must be trivially copyable");

static_assert(std::is_standard_layout_v<Execution>, "Execution must be standard layout");
static_assert(std::is_trivially_copyable_v<Execution>, "Execution must be trivially copyable");

static_assert(std::is_standard_layout_v<OrderRequest>, "OrderRequest must be standard layout");
static_assert(std::is_trivially_copyable_v<OrderRequest>, "OrderRequest must be trivially copyable");


// ============================================================
// HARD RULES (design constraints)
// ============================================================
//
// - No virtual functions → breaks standard layout
// - No non-trivial constructors → breaks trivial copyability
// - No implicit padding → all padding explicit
// - No floating-point → fixed-point only
// - Any change here = ABI break across system
//
// ============================================================

} // namespace core::common