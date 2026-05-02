// PoolAllocator.cpp
// Explicit template instantiations for all PoolAllocator types used in SAGE.
//
// PURPOSE:
//   All method bodies live in PoolAllocator.hpp for ThinLTO inlining.
//   These explicit instantiations tell the linker exactly which
//   specialisations exist, preventing duplicate symbol emission across
//   translation units that include the header.
//
// RULE:
//   Every PoolAllocator<T, N> used anywhere in the engine must have
//   a corresponding explicit instantiation here.
//   If you add a new pool type, add its instantiation below.

#include "PoolAllocator.hpp"

#include "../../core/common/Types.hpp"
#include "../../core/common/Constants.hpp"

namespace core::memory {

// ── Production instantiations ─────────────────────────────────────────────

// Order pool — holds all resting orders during the trading session
template class PoolAllocator<core::common::Order,     core::common::MAX_ORDERS>;

// Execution pool — holds execution records until written to shm
template class PoolAllocator<core::common::Execution, core::common::MAX_EXECUTIONS>;

// ── Notes ─────────────────────────────────────────────────────────────────
//
// OrderNode and OrderRequest pools are instantiated here once
// their types are defined in Phase 4 (orderbook/) and Phase 6 (feed/).
// Add them here at that point:
//
//   template class PoolAllocator<core::orderbook::OrderNode, core::common::MAX_ORDERS>;
//   template class PoolAllocator<core::common::OrderRequest, core::common::MAX_ORDERS>;

} // namespace core::memory