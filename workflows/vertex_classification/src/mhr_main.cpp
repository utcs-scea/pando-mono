// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include <cstdint>
#include <iostream>

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-wf1/mhr_ref.hpp"
#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/graphs/projection.hpp>
#include <pando-lib-galois/utility/timer.hpp>
#include <pando-rt/memory_resource.hpp>
#include <pando-wf1/graphs/mhr_graph.hpp>

#define TURING_AWARD  11020773
#define DEEP_LEARNING 12090508
#define TOP_K         50

namespace {

void printUsageExit(char* argv0) {
  std::printf(
      "Usage: %s "
      "-e <entity features csv file> "
      "-r <relation features csv file> "
      "-g <graph topology csv file> \n",
      argv0);
  std::exit(1);
}

struct ProgramOptions {
  ProgramOptions() = default;

  void Parse(int argc, char** argv) {
    PANDO_CHECK(nodeFiles.initialize(0));
    PANDO_CHECK(edgeFiles.initialize(0));
    optind = 0;

    int opt;
    const char* file;
    while ((opt = getopt(argc, argv, "e:r:g:")) != -1) {
      switch (opt) {
        case 'e':
          file = optarg;
          std::printf("Entity file: %s\n", file);
          PANDO_CHECK(nodeFiles.pushBack(galois::VertexParser<wf1::MHRNode>(
              galois::StringView(file).toArray(), wf1::ParseEmbeddingLine, ',')));
          break;
        case 'r':
          relationFile = optarg;
          break;
        case 'g':
          file = optarg;
          std::printf("Topology file: %s\n", file);
          PANDO_CHECK(edgeFiles.pushBack(galois::EdgeParser<wf1::MHREdge>(
              galois::StringView(file).toArray(), wf1::ParseRelationLine, ',')));
          break;
        default:
          printUsageExit(argv[0]);
          PANDO_ABORT("invalid arguments");
      }
    }
    if (Verify() != pando::Status::Success) {
      printUsageExit(argv[0]);
      PANDO_ABORT("invalid arguments");
    }
    std::cout << "Relation file: " << relationFile << std::endl;
  }

  pando::Status Verify() {
    if (nodeFiles.empty()) {
      return pando::Status::InvalidValue;
    }
    if (edgeFiles.empty()) {
      return pando::Status::InvalidValue;
    }
    if (relationFile == nullptr) {
      return pando::Status::InvalidValue;
    }
    return pando::Status::Success;
  }

  wf1::NodeFiles nodeFiles;
  wf1::EdgeFiles edgeFiles;
  const char* relationFile{nullptr};
} typedef ProgramOptions;

void checkGraph(wf1::MHRGraph& graph) {
  uint64_t vertices = 0;
  uint64_t edges = 0;
  for (wf1::MHRGraph::VertexTopologyID node : graph.vertices()) {
    vertices++;
    wf1::MHRNode nodeData = graph.getData(node);
    for (wf1::MHRGraph::EdgeHandle edge : graph.edges(node)) {
      edges++;
      wf1::MHREdge edgeData = graph.getEdgeData(edge);
      std::cout << "Edge: " << edgeData.src << "," << edgeData.type << "," << edgeData.dst << ","
                << edgeData.mirror << std::endl;
      if (nodeData.id != edgeData.src) {
        std::cout << "Mismatch src id: " << nodeData.id << std::endl;
      }
      wf1::MHRNode dstData = graph.getData(graph.getEdgeDst(edge));
      if (dstData.id != edgeData.dst) {
        std::cout << "Mismatch dst id: " << dstData.id << std::endl;
      }
    }
  }
  std::cout << "Counted nodes: " << vertices << std::endl;
  std::cout << "Counted edges: " << edges << std::endl;
}

} // namespace

int pandoMain(int argc, char** argv) {
  if (pando::getCurrentPlace().node.id == 0) {
    galois::Timer workflow_timer("Start workflow 1 Multi-Hop Reasoning",
                                 "Finished workflow 1 Multi-Hop Reasoning");

    ProgramOptions programOptions;
    programOptions.Parse(argc, argv);

    galois::Timer import_timer("Start import", "Finished import");
    wf1::MHRGraph fullGraph{};
    fullGraph.initialize(std::move(programOptions.nodeFiles), std::move(programOptions.edgeFiles));
    wf1::RelationFeatures relationFeatures;
    PANDO_CHECK(relationFeatures.initialize(galois::VertexParser<wf1::MHRNode>(
        galois::StringView(programOptions.relationFile).toArray(), wf1::ParseEmbeddingLine, ',')));
    import_timer.Stop();
    std::printf("Full Graph Nodes: %lu\n", fullGraph.size());
    std::printf("Full Graph Edges: %lu\n", fullGraph.sizeEdges());

    galois::Timer projection_timer("Start projection", "Finished projection");
    wf1::MHRGraph projectedGraph =
        galois::Project<wf1::MHRGraph, wf1::MHRGraph,
                        wf1::internal::MHRGraphProjection<wf1::MHRGraph>>(
            std::move(fullGraph), wf1::internal::MHRGraphProjection<wf1::MHRGraph>());
    projection_timer.Stop();

#ifdef PRINT_GRAPH
    checkGraph(projectedGraph);
#endif
    mhr_ref::MHR_REF<wf1::MHRGraph> mhr;
    pando::Vector<std::uint64_t> results;
    pando::Vector<std::uint64_t> universities;

    galois::Timer award_timer("Start computing award scores", "Finished computing award scores");
    results = mhr.computeScores(projectedGraph, relationFeatures, wf1::MHR_ENTITY::PERSON,
                                wf1::AWARD_WINNER_TYPE, TURING_AWARD);
    award_timer.Stop();

    for (std::uint64_t i = 0; i < results.size(); i++) {
      uint64_t person = results[i];
      printf("Person %ld: %ld\n", i + 1, person);
    }

    galois::Timer field_timer("Start sort by field scores", "Finished sort by field scores");
    results = mhr.computeVertexScores(projectedGraph, relationFeatures, std::move(results),
                                      wf1::WORKS_IN_TYPE, DEEP_LEARNING);
    field_timer.Stop();

    for (std::uint64_t i = 0; i < results.size(); i++) {
      uint64_t person = results[i];
      printf("Person %ld: %ld\n", i + 1, person);
    }

    galois::Timer university_timer("Start computing university scores",
                                   "Finished computing university scores");
    for (auto person_id : results) {
      universities =
          mhr.computeScores(projectedGraph, relationFeatures, wf1::MHR_ENTITY::UNIVERSITY,
                            wf1::AFFILIATED_WITH_TYPE, person_id);
      std::uint64_t university = universities[0];
      wf1::MHRNode A = projectedGraph.getData(projectedGraph.getTopologyID(person_id));
      wf1::MHRNode B = projectedGraph.getData(projectedGraph.getTopologyID(university));
      printf("%ld Person %ld University\n", A.id, B.id);
    }
    university_timer.Stop();
  }
  pando::waitAll();
  return 0;
}
