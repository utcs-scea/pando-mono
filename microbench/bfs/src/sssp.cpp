#include <pando-bfs-galois/sssp.hpp>

void galois::updateData(std::uint64_t val, pando::GlobalRef<std::uint64_t> ref) {
  std::uint64_t temp = pando::atomicLoad(&ref, std::memory_order_relaxed);
  do {
    if (val >= temp) {
      break;
    }
  } while (!pando::atomicCompareExchange(&ref, pando::GlobalPtr<std::uint64_t>(&temp),
                                         pando::GlobalPtr<std::uint64_t>(&val),
                                         std::memory_order_relaxed, std::memory_order_relaxed));
}

galois::CountEdges<galois::COUNT_EDGE> countEdges;
