// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/ingest_wmd_csv.hpp>

auto generateWMDParser(
    pando::Array<galois::StringView> tokens,
    pando::GlobalPtr<pando::Vector<pando::Vector<galois::WMDEdge>>> localEdges,
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> localRename,
    pando::GlobalPtr<pando::Vector<galois::WMDVertex>> localReadVertices, uint64_t* totVerts) {
  using galois::WMDEdge, galois::WMDVertex;
  using galois::internal::insertLocalEdgesPerThread;
  return [localEdges, localRename, localReadVertices, totVerts, tokens](char* line) {
    auto vfunc = [localReadVertices, totVerts](WMDVertex v) {
      *totVerts += 1;
      return fmap(*localReadVertices, pushBack, v);
    };
    auto efunc = [localEdges, localRename](WMDEdge e, agile::TYPES inverseEdgeType) {
      WMDEdge inverseE = e;
      inverseE.type = inverseEdgeType;
      std::swap(inverseE.src, inverseE.dst);
      std::swap(inverseE.srcType, inverseE.dstType);
      PANDO_CHECK_RETURN(insertLocalEdgesPerThread(*localRename, *localEdges, e));
      PANDO_CHECK_RETURN(insertLocalEdgesPerThread(*localRename, *localEdges, inverseE));
      return pando::Status::Success;
    };
    return galois::wmdCSVParse(line, tokens, vfunc, efunc);
  };
}

void galois::loadWMDFilePerThread(
    galois::WaitGroup::HandleType wgh, pando::Array<char> filename, std::uint64_t segmentsPerThread,
    std::uint64_t numThreads, std::uint64_t threadID,
    galois::PerThreadVector<pando::Vector<WMDEdge>> localEdges,
    galois::ThreadLocalStorage<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename,
    galois::ThreadLocalVector<WMDVertex> localReadVertices,
    galois::DAccumulator<std::uint64_t> totVerts) {
  std::uint64_t countLocalVertices = 0;
  pando::Array<galois::StringView> tokens;
  PANDO_CHECK(tokens.initialize(10));
  auto parser = generateWMDParser(tokens, &localEdges.getThreadVector(), perThreadRename.getLocal(),
                                  localReadVertices.getLocal(), &countLocalVertices);
  PANDO_CHECK(
      internal::loadGraphFilePerThread(filename, segmentsPerThread, numThreads, threadID, parser));

  totVerts.add(countLocalVertices);
  wgh.done();
  tokens.deinitialize();
}
