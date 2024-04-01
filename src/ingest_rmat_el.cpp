// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <pando-lib-galois/import/ingest_rmat_el.hpp>

auto generateRMATParser(
    pando::GlobalPtr<pando::Vector<pando::Vector<galois::ELEdge>>> localEdges,
    pando::GlobalPtr<galois::HashTable<std::uint64_t, std::uint64_t>> localRename,
    std::uint64_t numVertices) {
  using galois::ELEdge;
  using galois::internal::insertLocalEdgesPerThread;
  return [localEdges, localRename, numVertices](char* line) {
    auto efunc = [localEdges, localRename, numVertices](std::uint64_t src, std::uint64_t dst) {
      if (src < numVertices && dst < numVertices) {
        return insertLocalEdgesPerThread(*localRename, *localEdges, ELEdge{src, dst});
      }
      return pando::Status::Success;
    };
    return galois::elParse(line, efunc);
  };
}

void galois::loadELFilePerThread(
    galois::WaitGroup::HandleType wgh, pando::Array<char> filename, std::uint64_t segmentsPerThread,
    std::uint64_t numThreads, std::uint64_t threadID,
    galois::PerThreadVector<pando::Vector<ELEdge>> localEdges,
    ThreadLocalStorage<HashTable<std::uint64_t, std::uint64_t>> perThreadRename,
    std::uint64_t numVertices) {
  auto parser =
      generateRMATParser(&localEdges.getThreadVector(), perThreadRename.getLocal(), numVertices);
  PANDO_CHECK(
      internal::loadGraphFilePerThread(filename, segmentsPerThread, numThreads, threadID, parser));
  wgh.done();
}

const char* galois::elGetOne(const char* line, std::uint64_t& val) {
  bool found = false;
  val = 0;
  char c;
  while ((c = *line++) != '\0' && isspace(c)) {}
  do {
    if (isdigit(c)) {
      found = true;
      val *= 10;
      val += (c - '0');
    } else if (c == '_') {
      continue;
    } else {
      break;
    }
  } while ((c = *line++) != '\0' && !isspace(c));
  if (!found)
    val = UINT64_MAX;
  return line;
}

pando::Status galois::generateEdgesPerVirtualHost(
    pando::GlobalRef<pando::Vector<ELVertex>> vertices, std::uint64_t totalVertices,
    std::uint64_t vHostID, std::uint64_t numVHosts) {
  const std::uint64_t oldSize = lift(vertices, size);
  const bool addOne = ((totalVertices % numVHosts) < vHostID);
  const std::uint64_t deltaSize = (totalVertices / numVHosts) + (addOne ? 1 : 0);
  PANDO_CHECK_RETURN(fmap(vertices, reserve, oldSize + deltaSize));
  for (std::uint64_t i = vHostID; i < totalVertices; i += numVHosts) {
    PANDO_CHECK_RETURN(fmap(vertices, pushBack, ELVertex{i}));
  }
  return pando::Status::Success;
}

pando::Vector<pando::Vector<galois::ELEdge>> galois::reduceLocalEdges(
    galois::PerThreadVector<pando::Vector<galois::ELEdge>> localEdges, uint64_t numVertices) {
  pando::Vector<pando::Vector<galois::ELEdge>> reducedEL;
  PANDO_CHECK(reducedEL.initialize(numVertices));

  for (uint64_t i = 0; i < numVertices; i++) {
    pando::Vector<galois::ELEdge> ev = reducedEL[i];
    PANDO_CHECK(ev.initialize(0));
    reducedEL[i] = std::move(ev);
  }

  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<pando::Vector<galois::ELEdge>> threadLocalEdges = *localEdges.get(i);
    for (pando::Vector<galois::ELEdge> ev : threadLocalEdges) {
      if (ev.size() > 0) {
        ELEdge first_edge = ev[0];
        uint64_t src = first_edge.src;
        pando::Vector<galois::ELEdge> src_ev = reducedEL[src];
        PANDO_CHECK(src_ev.append(&ev));
        reducedEL[src] = std::move(src_ev);
      }
    }
  }

  galois::doAll(
      reducedEL, +[](pando::Vector<galois::ELEdge> src_ev) {
        std::sort(src_ev.begin(), src_ev.end());
      });
  return reducedEL;
}
