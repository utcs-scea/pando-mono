// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <stdbool.h>

#include <pando-rt/export.h>
#include <pando-wf4/export.h>

#include <pando-lib-galois/containers/host_local_storage.hpp>
#include <pando-lib-galois/containers/pod_local_storage.hpp>
#include <pando-lib-galois/utility/timer.hpp>
#include <pando-rt/pando-rt.hpp>
#include <pando-wf4/graph.hpp>
#include <pando-wf4/import.hpp>
#include <pando-wf4/influence_maximization.hpp>

namespace {

void printUsageExit(char* argv0) {
  std::printf(
      "Usage: %s "
      "-k <num-influential-nodes> "
      "-r <number-reverse-reachable-sets> "
      "[-s <random-seed>] "
      "-c <commercial-path> "
      "-y <cyber-path> "
      "-o <social-path> "
      "-u <uses-path> "
      "[-2 <0 disables kernel 2 (Projection)>]"
      "[-2 <0 disables kernel 3 (Influence Maximization)>]\n",
      argv0);
  std::exit(EXIT_FAILURE);
}

struct ProgramOptions {
  ProgramOptions() = default;

  void Parse(int argc, char** argv) {
    PANDO_CHECK(input_files.initialize(0));
    // Other libraries may have called getopt before, so we reset optind for correctness
    optind = 0;

    int opt;
    const char* file;
    while ((opt = getopt(argc, argv, "k:r:s:c:y:o:u:n:2:3:")) != -1) {
      switch (opt) {
        case 'k':
          k = strtoull(optarg, nullptr, 10);
          break;
        case 'r':
          rrr = strtoull(optarg, nullptr, 10);
          break;
        case 's':
          seed = strtoull(optarg, nullptr, 10);
          break;
        case 'c':
          file = optarg;
          std::printf("Commercial file: %s\n", file);
          PANDO_CHECK(input_files.pushBack(galois::EdgeParser<wf4::FullNetworkEdge>(
              galois::StringView(file).toArray(), wf4::ParseCommercialLine)));
          break;
        case 'y':
          file = optarg;
          std::printf("Cyber file: %s\n", file);
          PANDO_CHECK(input_files.pushBack(galois::EdgeParser<wf4::FullNetworkEdge>(
              galois::StringView(file).toArray(), wf4::ParseCyberLine)));
          break;
        case 'o':
          file = optarg;
          std::printf("Social file: %s\n", file);
          PANDO_CHECK(input_files.pushBack(galois::EdgeParser<wf4::FullNetworkEdge>(
              galois::StringView(file).toArray(), wf4::ParseSocialLine)));
          break;
        case 'u':
          file = optarg;
          std::printf("Uses file: %s\n", file);
          PANDO_CHECK(input_files.pushBack(galois::EdgeParser<wf4::FullNetworkEdge>(
              galois::StringView(file).toArray(), wf4::ParseUsesLine)));
          break;
        case '2':
          disable_kernel2 = strtoll(optarg, nullptr, 10) <= 0;
          break;
        case '3':
          disable_kernel3 = strtoll(optarg, nullptr, 10) <= 0;
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
  }

  pando::Status Verify() {
    if (k <= 0) {
      return pando::Status::InvalidValue;
    }
    if (rrr <= 0) {
      return pando::Status::InvalidValue;
    }
    if (input_files.size() == 0) {
      return pando::Status::InvalidValue;
    }
    return pando::Status::Success;
  }

  uint64_t k = 0;
  uint64_t rrr = 0;
  uint64_t seed = 9801;
  wf4::InputFiles input_files;
  bool disable_kernel2 = false;
  bool disable_kernel3 = false;
} typedef ProgramOptions;

#ifdef DIST_ARRAY_CSR
void checkGraph(wf4::FullNetworkGraph& graph) {
  uint64_t counted_nodes = 0;
  uint64_t counted_edges = 0;
  uint64_t last_token_node_id = 0;
  for (wf4::FullNetworkGraph::VertexTopologyID node : graph.vertices()) {
    counted_nodes++;
    counted_edges += graph.getNumEdges(node);
    wf4::FullNetworkNode node_data = graph.getData(node);
    for (auto eh : graph.edges(node)) {
      wf4::FullNetworkEdge edge_data = graph.getEdgeData(eh);
      wf4::FullNetworkGraph::VertexTopologyID edge_dst = graph.getEdgeDst(eh);
      wf4::FullNetworkNode dst_data = graph.getData(edge_dst);
      if (node_data.id != edge_data.src) {
        std::printf("Error: bad source id %lu, expected %lu, destination is %lu\n", node_data.id,
                    edge_data.src, edge_data.dst);
      }
      if (dst_data.id != edge_data.dst) {
        std::printf("Error: bad destination id %lu, expected %lu, source is %lu\n", dst_data.id,
                    edge_data.dst, edge_data.src);
      }
    }
  }
}
#endif

} // namespace

int pandoMain(int argc, char** argv) {
  if (pando::getCurrentPlace().node.id == 0) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();

    galois::Timer workflow_timer("Start workflow 4", "Finished workflow 4");

    ProgramOptions program_options;
    program_options.Parse(argc, argv);

    galois::Timer import_timer("Start import", "Finished import");
    wf4::FullNetworkGraph full_graph = wf4::ImportData(std::move(program_options.input_files));
    import_timer.Stop();
    std::printf("Full Graph Nodes: %lu\n", full_graph.size());
    std::printf("Full Graph Edges: %lu\n", full_graph.sizeEdges());

#ifdef DIST_ARRAY_CSR
    checkGraph(full_graph);
#endif

    if (program_options.disable_kernel2) {
      pando::waitAll();
      return 0;
    }

    galois::Timer projection_timer("Start projection", "Finished projection");
    wf4::NetworkGraph graph = wf4::ProjectGraph(full_graph);
    projection_timer.Stop();

    if (program_options.disable_kernel3) {
      pando::waitAll();
      return 0;
    }

    wf4::CalculateEdgeProbabilities(graph);
    galois::Timer rrr_timer("Start generating RRR sets", "Finished generating RRR sets");
    wf4::ReverseReachableSet rrr_sets =
        wf4::GetRandomReverseReachableSets(graph, program_options.rrr, program_options.seed);
    rrr_timer.Stop();
    galois::Timer influential_timer("Start finding influential nodes",
                                    "Finished finding influential nodes");
    pando::Vector<wf4::NetworkGraph::VertexTokenID> influential_nodes =
        wf4::GetInfluentialNodes(graph, std::move(rrr_sets), program_options.k);
    influential_timer.Stop();

    workflow_timer.Stop();
  }
  pando::waitAll();
  return 0;
}
