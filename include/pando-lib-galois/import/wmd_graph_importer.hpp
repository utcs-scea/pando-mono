// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

#include <pando-lib-galois/containers/hashtable.hpp>
#include <pando-lib-galois/containers/per_host.hpp>
#include <pando-lib-galois/containers/per_thread.hpp>
#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-lib-galois/import/schema.hpp>
#include <pando-lib-galois/utility/dist_accumulator.hpp>
#include <pando-lib-galois/utility/gptr_monad.hpp>
#include <pando-lib-galois/utility/pair.hpp>
#include <pando-lib-galois/utility/string_view.hpp>
#include <pando-lib-galois/utility/tuple.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-rt/sync/notification.hpp>

namespace galois::internal {

/**
 * @brief This is used to parallel insert Edges
 */
template <typename EdgeType>
[[nodiscard]] inline pando::Status insertLocalEdgesPerThread(
    pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef,
    pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> localEdges, EdgeType edge) {
  uint64_t result;

  if (fmap(hashRef, get, edge.src, result)) {
    pando::GlobalRef<pando::Vector<EdgeType>> vec = fmap(localEdges, get, result);
    return fmap(vec, pushBack, edge);
  } else {
    PANDO_CHECK_RETURN(fmap(hashRef, put, edge.src, lift(localEdges, size)));
    pando::Vector<EdgeType> v;
    PANDO_CHECK_RETURN(v.initialize(1));
    v[0] = std::move(edge);
    return fmap(localEdges, pushBack, std::move(v));
  }
}

/**
 * @brief fills out the metadata for the VirtualToPhysical Host mapping.
 */
template <typename EdgeType>
[[nodiscard]] pando::Status buildEdgeCountToSend(
    std::uint64_t numVirtualHosts, galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::GlobalRef<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts) {
  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> sumArray;
  PANDO_CHECK_RETURN(sumArray.initialize(numVirtualHosts));

  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    galois::Pair<std::uint64_t, std::uint64_t> p{0, i};
    sumArray[i] = p;
  }

  PANDO_CHECK_RETURN(galois::doAll(
      sumArray, localEdges,
      +[](pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> counts,
          pando::Vector<pando::Vector<EdgeType>> localEdges) {
        for (pando::Vector<EdgeType> v : localEdges) {
          assert(v.size() != 0);
          EdgeType e = v[0];
          pando::GlobalPtr<char> toAdd = static_cast<pando::GlobalPtr<char>>(
              static_cast<pando::GlobalPtr<void>>(&counts[e.src % counts.size()]));
          using UPair = galois::Pair<std::uint64_t, std::uint64_t>;
          toAdd += offsetof(UPair, first);
          pando::GlobalPtr<std::uint64_t> p = static_cast<pando::GlobalPtr<std::uint64_t>>(
              static_cast<pando::GlobalPtr<void>>(toAdd));
          pando::atomicFetchAdd(p, v.size(), std::memory_order_relaxed);
        }
      }));
  labeledEdgeCounts = sumArray;
  return pando::Status::Success;
}

/**
 * @brief fills out the metadata for the VirtualToPhysical Host mapping.
 */
template <typename EdgeType>
void buildEdgeCountToSend(
    std::uint64_t numVirtualHosts, galois::PerThreadVector<EdgeType> localEdges,
    pando::GlobalRef<pando::Array<galois::Pair<std::uint64_t, std::uint64_t>>> labeledEdgeCounts) {
  pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> sumArray;
  PANDO_CHECK(sumArray.initialize(numVirtualHosts));

  for (std::uint64_t i = 0; i < numVirtualHosts; i++) {
    galois::Pair<std::uint64_t, std::uint64_t> p{0, i};
    sumArray[i] = p;
  }
  galois::WaitGroup wg{};
  PANDO_CHECK(wg.initialize(0));
  auto wgh = wg.getHandle();

  using UPair = galois::Pair<std::uint64_t, std::uint64_t>;
  const uint64_t pairOffset = offsetof(UPair, first);

  auto state = galois::make_tpl(wgh, sumArray);
  PANDO_CHECK(galois::doAll(
      wgh, state, localEdges, +[](decltype(state) state, pando::Vector<EdgeType> localEdges) {
        auto [wgh, sumArray] = state;
        PANDO_CHECK(galois::doAll(
            wgh, sumArray, localEdges, +[](decltype(sumArray) counts, EdgeType localEdge) {
              pando::GlobalPtr<char> toAdd = static_cast<pando::GlobalPtr<char>>(
                  static_cast<pando::GlobalPtr<void>>(&counts[localEdge.src % counts.size()]));
              toAdd += pairOffset;
              pando::GlobalPtr<std::uint64_t> p = static_cast<pando::GlobalPtr<std::uint64_t>>(
                  static_cast<pando::GlobalPtr<void>>(toAdd));
              pando::atomicFetchAdd(p, 1, std::memory_order_relaxed);
            }));
      }));
  PANDO_CHECK(wg.wait());
  labeledEdgeCounts = sumArray;
}

[[nodiscard]] pando::Status buildVirtualToPhysicalMapping(
    std::uint64_t numHosts,
    pando::Array<galois::Pair<std::uint64_t, std::uint64_t>> labeledVirtualCounts,
    pando::GlobalPtr<pando::Array<std::uint64_t>> virtualToPhysicalMapping,
    pando::Array<std::uint64_t> numEdges);

inline std::uint64_t getPhysical(std::uint64_t id,
                                 pando::Array<std::uint64_t> virtualToPhysicalMapping) {
  return virtualToPhysicalMapping[id % virtualToPhysicalMapping.size()];
}

/**
 * @brief PerHostVertex decomposition by partition
 *
 * TODO(AdityaAtulTewari) parallelize this
 */
template <typename VertexType>
[[nodiscard]] pando::Status perHostPartitionVertex(
    pando::Array<std::uint64_t> virtualToPhysicalMapping, pando::Vector<VertexType> vertices,
    pando::GlobalPtr<galois::PerHost<pando::Vector<VertexType>>> partitionedVertices) {
  galois::PerHost<pando::Vector<VertexType>> partitioned;
  PANDO_CHECK_RETURN(partitioned.initialize());

  std::uint64_t initSize = vertices.size() / lift(*partitionedVertices, size);
  for (pando::GlobalRef<pando::Vector<VertexType>> vec : partitioned) {
    PANDO_CHECK_RETURN(fmap(vec, initialize, 0));
    PANDO_CHECK_RETURN(fmap(vec, reserve, initSize));
  }

  for (VertexType vert : vertices) {
    PANDO_CHECK_RETURN(
        fmap(partitioned.get(getPhysical(vert.id, virtualToPhysicalMapping)), pushBack, vert));
  }

  *partitionedVertices = partitioned;
  return pando::Status::Success;
}

/**
 * @brief Serially build the edgeLists
 */
template <typename EdgeType>
[[nodiscard]] pando::Status partitionEdgesSerially(
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    pando::Array<std::uint64_t> virtualToPhysicalMapping,
    galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partitionedEdges,
    galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost) {
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    PANDO_CHECK_RETURN(hash.initialize(0));
    hashRef = hash;
  }
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<pando::Vector<EdgeType>> threadLocalEdges = *localEdges.get(i);
    for (pando::Vector<EdgeType> vec : threadLocalEdges) {
      for (EdgeType edge : vec) {
        auto tgtHost = getPhysical(edge.src, virtualToPhysicalMapping);
        pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> edges =
            partitionedEdges.get(tgtHost);
        PANDO_CHECK_RETURN(insertLocalEdgesPerThread(renamePerHost.get(tgtHost), edges, edge));
      }
    }
  }
  return pando::Status::Success;
}

/**
 * @brief Serially build the edgeLists
 */
template <typename EdgeType>
[[nodiscard]] pando::Status partitionEdgesSerially(
    galois::PerThreadVector<EdgeType> localEdges,
    pando::Array<std::uint64_t> virtualToPhysicalMapping,
    galois::PerHost<pando::Vector<pando::Vector<EdgeType>>> partitionedEdges,
    galois::PerHost<galois::HashTable<std::uint64_t, std::uint64_t>> renamePerHost) {
  for (pando::GlobalRef<galois::HashTable<std::uint64_t, std::uint64_t>> hashRef : renamePerHost) {
    galois::HashTable<std::uint64_t, std::uint64_t> hash(.8);
    PANDO_CHECK_RETURN(hash.initialize(0));
    hashRef = hash;
  }
  for (std::uint64_t i = 0; i < localEdges.size(); i++) {
    pando::Vector<EdgeType> threadLocalEdges = *localEdges.get(i);
    for (EdgeType edge : threadLocalEdges) {
      auto tgtHost = getPhysical(edge.src, virtualToPhysicalMapping);
      pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> edges =
          partitionedEdges.get(tgtHost);
      PANDO_CHECK_RETURN(insertLocalEdgesPerThread(renamePerHost.get(tgtHost), edges, edge));
    }
  }
  return pando::Status::Success;
}

/**
 * @brief Consumes the localVertices, and references a partitionMap to produce partitioned Vertices
 */
template <typename VertexType>
[[nodiscard]] galois::PerHost<pando::Vector<VertexType>> partitionVertices(
    galois::PerThreadVector<VertexType>&& vertexPerThreadRead, pando::Array<std::uint64_t> v2PM) {
  galois::PerHost<pando::Vector<VertexType>> vertPart{};
  PANDO_CHECK(vertPart.initialize());

  galois::PerHost<pando::Vector<VertexType>> readPart{};
  PANDO_CHECK(readPart.initialize());
  for (pando::GlobalRef<pando::Vector<VertexType>> vec : readPart) {
    PANDO_CHECK(fmap(vec, initialize, 0));
  }
  PANDO_CHECK(vertexPerThreadRead.hostFlattenAppend(readPart));

#if FREE
  auto freeLocalVertices = +[](PerThreadVector<VertexType> localVertices) {
    localVertices.deinitialize();
  };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freeLocalVertices, localVertices));
#endif

  galois::PerHost<galois::PerHost<pando::Vector<VertexType>>> partVert{};
  PANDO_CHECK(partVert.initialize());

  struct PHPV {
    pando::Array<std::uint64_t> v2PM;
    PerHost<pando::Vector<VertexType>> pHV;
  };

  auto f = +[](PHPV phpv, pando::GlobalRef<PerHost<pando::Vector<VertexType>>> partVert) {
    const std::uint64_t hostID = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
    PANDO_CHECK(galois::internal::perHostPartitionVertex<VertexType>(
        phpv.v2PM, phpv.pHV.get(hostID), &partVert));
  };
  PANDO_CHECK(galois::doAll(PHPV{v2PM, readPart}, partVert, f));

#if FREE
  auto freeReadPart = +[](PerHost<pando::Vector<VertexType>> readPart) {
    for (pando::Vector<WMDVertex> vec : readPart) {
      vec.deinitialize();
    }
    readPart.deinitialize();
  };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freeReadPart, readPart));
#endif

  PANDO_CHECK(galois::doAll(
      partVert, vertPart,
      +[](decltype(partVert) pHPHV, pando::GlobalRef<pando::Vector<VertexType>> pHV) {
        PANDO_CHECK(fmap(pHV, initialize, 0));
        std::uint64_t currNode = static_cast<std::uint64_t>(pando::getCurrentPlace().node.id);
        for (galois::PerHost<pando::Vector<WMDVertex>> pHVRef : pHPHV) {
          PANDO_CHECK(fmap(pHV, append, &pHVRef.get(currNode)));
        }
      }));

#if FREE
  auto freePartVert = +[](PerHost<PerHost<pando::Vector<VertexType>>> partVert) {
    for (PerHost<pando::Vector<WMDVertex>> pVV : partVert) {
      for (pando::Vector<WMDVertex> vV : pVV) {
        vV.deinitialize();
      }
      pVV.deinitialize();
    }
    partVert.deinitialize();
  };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freePartVert, partVert));
#endif

  return vertPart;
}

/**
 * @brief Consumes localEdges, and references a partitionMap to produced partitioned Edges grouped
 * by Vertex, along with a rename set of Vertices
 */
template <typename EdgeType>
[[nodiscard]] Pair<PerHost<pando::Vector<pando::Vector<EdgeType>>>,
                   PerHost<HashTable<std::uint64_t, std::uint64_t>>>
partitionEdgesPerHost(PerThreadVector<pando::Vector<EdgeType>>&& localEdges,
                      pando::Array<std::uint64_t> v2PM) {
  PerHost<pando::Vector<pando::Vector<EdgeType>>> partEdges{};
  PANDO_CHECK(partEdges.initialize());

  for (pando::GlobalRef<pando::Vector<pando::Vector<EdgeType>>> vvec : partEdges) {
    PANDO_CHECK(fmap(vvec, initialize, 0));
  }

  PerHost<HashTable<std::uint64_t, std::uint64_t>> renamePerHost{};
  PANDO_CHECK(renamePerHost.initialize());

  PANDO_CHECK(galois::internal::partitionEdgesSerially(localEdges, v2PM, partEdges, renamePerHost));

#if FREE
  auto freeLocalEdges = +[](PerThreadVector<pando::Vector<EdgeType>> localEdges) {
    for (pando::Vector<pando::Vector<EdgeType>> vVE : localEdges) {
      for (pando::Vector<EdgeType> vE : vVE) {
        vE.deinitialize();
      }
    }
    localEdges.deinitialize();
  };
  PANDO_CHECK(pando::executeOn(pando::anyPlace, freeLocalEdges, localEdges));
#endif
  return galois::make_tpl(partEdges, renamePerHost);
}

uint64_t inline getFileReadOffset(galois::ifstream& file, uint64_t segment, uint64_t numSegments) {
  uint64_t fileSize = file.size();
  if (segment == 0) {
    return 0;
  }
  if (segment >= numSegments) {
    return fileSize;
  }

  uint64_t bytesPerSegment = fileSize / numSegments;
  uint64_t offset = segment * bytesPerSegment;
  pando::Vector<char> line;
  PANDO_CHECK(line.initialize(0));

  // check for partial line at start
  file.seekg(offset - 1);
  file.getline(line, '\n');
  // if not at start of a line, discard partial line
  if (!line.empty())
    offset += line.size();
  line.deinitialize();
  return offset;
}

/*
 * Load graph info from the file.
 *
 * @param filename loaded file for the graph
 * @param segmentsPerHost the number of file segments each host will load.
 * If value is 1, no file striping is performed. The file is striped into
 * (segementsPerHost * numHosts) segments.
 *
 * @details File striping is used to randomize the order of nodes/edges
 * loaded from the graph. WMD dataset csv typically grouped nodes/edges by its types,
 * which will produce an imbalanced graph if you break the file evenly among hosts.
 * So file striping make each host be able to load multiple segments in different positions of the
 * file, which produced a more balanced graph.
 *
 * @note per thread method
 */
template <typename ParseFunc>
pando::Status loadGraphFilePerThread(pando::Array<char> filename, uint64_t segmentsPerThread,
                                     std::uint64_t numThreads, std::uint64_t threadID,
                                     ParseFunc parseFunc) {
  galois::ifstream graphFile;
  PANDO_CHECK_RETURN(graphFile.open(filename));
  uint64_t numSegments = numThreads * segmentsPerThread;

  // init per thread data struct

  // for each host N, it will read segment like:
  // N, N + numHosts, N + numHosts * 2, ..., N + numHosts * (segmentsPerHost - 1)
  for (uint64_t cur = 0; cur < segmentsPerThread; cur++) {
    uint64_t segmentID = (uint64_t)threadID + cur * numThreads;
    uint64_t start = getFileReadOffset(graphFile, segmentID, numSegments);
    uint64_t end = getFileReadOffset(graphFile, segmentID + 1, numSegments);
    if (start == end) {
      continue;
    }
    graphFile.seekg(start);

    // load segment into memory
    uint64_t segmentLength = end - start;
    char* segmentBuffer = new char[segmentLength];
    graphFile.read(segmentBuffer, segmentLength);

    // A parallel loop that parse the segment
    // task 1: get token to global id mapping
    // task 2: get token to edges mapping
    char* currentLine = segmentBuffer;
    char* endLine = currentLine + segmentLength;

    while (currentLine < endLine) {
      assert(std::strchr(currentLine, '\n'));
      char* nextLine = std::strchr(currentLine, '\n') + 1;

      // skip comments
      if (currentLine[0] == '#') {
        currentLine = nextLine;
        continue;
      }

      PANDO_CHECK_RETURN(parseFunc(currentLine));
      currentLine = nextLine;
    }
    delete[] segmentBuffer;
  }
  graphFile.close();
  return pando::Status::Success;
}

/*
 * Load vertex info from a parser
 */
template <typename VertexType>
void loadVertexFilePerThread(pando::NotificationHandle done,
                             galois::VertexParser<VertexType> parser, uint64_t segmentsPerThread,
                             std::uint64_t numThreads, std::uint64_t threadID,
                             galois::PerThreadVector<VertexType> localVertices) {
  pando::GlobalRef<pando::Vector<VertexType>> localVert = localVertices.getThreadVector();
  auto parseLine = [&parser, &localVert](const char* currentLine) {
    if (currentLine[0] != parser.comment) {
      PANDO_CHECK_RETURN(fmap(localVert, pushBack, parser.parser(currentLine)));
    }
    return pando::Status::Success;
  };
  PANDO_CHECK(
      loadGraphFilePerThread(parser.filename, segmentsPerThread, numThreads, threadID, parseLine));
  done.notify();
}

template <typename EdgeType>
void loadEdgeFilePerThread(
    pando::NotificationHandle done, galois::EdgeParser<EdgeType> parser, uint64_t segmentsPerThread,
    std::uint64_t numThreads, std::uint64_t threadID,
    galois::PerThreadVector<pando::Vector<EdgeType>> localEdges,
    galois::DistArray<galois::HashTable<std::uint64_t, std::uint64_t>> perThreadRename) {
  auto hartID = localEdges.getLocalVectorID();
  auto localEdgeVec = localEdges.getThreadVector();
  auto hashRef = perThreadRename[hartID];

  auto parseLine = [&parser, &localEdgeVec, &hashRef](const char* currentLine) {
    if (currentLine[0] != parser.comment) {
      ParsedEdges<EdgeType> parsed = parser.parser(currentLine);
      if (parsed.isEdge) {
        PANDO_CHECK_RETURN(insertLocalEdgesPerThread(hashRef, localEdgeVec, parsed.edge1));
        if (parsed.has2Edges) {
          PANDO_CHECK_RETURN(insertLocalEdgesPerThread(hashRef, localEdgeVec, parsed.edge2));
        }
      }
    }
    return pando::Status::Success;
  };
  done.notify();
}

template <typename EdgeType>
struct ImportState {
  ImportState() = default;
  ImportState(galois::EdgeParser<EdgeType> parser_, galois::PerThreadVector<EdgeType> localEdges_)
      : parser(parser_), localEdges(localEdges_) {}

  galois::EdgeParser<EdgeType> parser;
  galois::PerThreadVector<EdgeType> localEdges;
};

template <typename EdgeType>
void loadGraphFile(ImportState<EdgeType>& state, uint64_t segmentID, uint64_t numSegments) {
  auto parseLine = [&state](const char* currentLine) {
    if (currentLine[0] == state.parser.comment) {
      return pando::Status::Success;
    }
    ParsedEdges<EdgeType> parsed = state.parser.parser(currentLine);
    if (parsed.isEdge) {
      PANDO_CHECK_RETURN(state.localEdges.pushBack(parsed.edge1));
      if (parsed.has2Edges) {
        PANDO_CHECK_RETURN(state.localEdges.pushBack(parsed.edge2));
      }
    }
    return pando::Status::Success;
  };
  PANDO_CHECK(loadGraphFilePerThread(state.parser.filename, 1, numSegments, segmentID, parseLine));
}

} // namespace galois::internal

#endif // PANDO_LIB_GALOIS_IMPORT_WMD_GRAPH_IMPORTER_HPP_
