// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-rt/sync/notification.hpp"
#include "pando-wf1/lp/graph_reader/import.hpp"
#include "pando-wf1/lp/lp_gnn.hpp"

#include <pando-lib-galois/graphs/dist_array_csr.hpp>
#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

using VertexType = wf::Vertex;
using EdgeType = wf::Edge;

namespace {

void printUsageExit(char* argv0) {
  std::printf(
      "Usage: %s "
      "-g <graph-path> "
      "-e <epochs>"
      "[-2 <0 disables kernel 2>]\n",
      argv0);
  std::exit(EXIT_FAILURE);
}

struct ProgramOptions {
  ProgramOptions() = default;

  void Parse(int argc, char** argv) {
    // Other libraries may have called getopt before, so we reset optind for correctness
    optind = 0;

    int opt;
    while ((opt = getopt(argc, argv, "g:e:2:")) != -1) {
      switch (opt) {
        case 'g':
          graphFile = optarg;
          break;
        case 'e':
          epochs = strtoull(optarg, nullptr, 10);
          break;
        case '2':
          disableKernel2 = strtoll(optarg, nullptr, 10) <= 0;
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
    if (graphFile == nullptr) {
      return pando::Status::InvalidValue;
    }
    return pando::Status::Success;
  }

  const char* graphFile{nullptr};
  std::uint64_t epochs{0};
  bool disableKernel2{false};
} typedef ProgramOptions;

} // namespace

void runGNN(pando::GlobalPtr<galois::DistArrayCSR<VertexType, EdgeType>> dGraphPtr,
            std::uint64_t numEpochs) {
  galois::DistArrayCSR<VertexType, EdgeType> g = *dGraphPtr;
  std::cout << "initialize completes ready to start gnn\n";
  gnn::LpGraphNeuralNetwork<std::uint64_t, std::uint64_t, VertexType, EdgeType> gnn;
  gnn.initialize(dGraphPtr);
  std::cout << "graph initialization completes\n";
  std::cout << "Epoch:" << numEpochs << " starts\n";
  gnn.train(numEpochs);
  std::cout << "graph train starts\n";
}

int pandoMain(int argc, char** argv) {
  const auto thisPlace = pando::getCurrentPlace();
  ProgramOptions programOptions;
  programOptions.Parse(argc, argv);
  if (thisPlace.node.id == 0) {
    const char* graphFile = programOptions.graphFile;
    pando::GlobalPtr<char> fname = static_cast<pando::GlobalPtr<char>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(char) * strlen(graphFile)));
    // TODO(hc): accessing the raw pointer of the global pointer is
    // not supported iiuc.
    for (size_t i = 0; i < strlen(graphFile); ++i) {
      fname[i] = graphFile[i];
    }

    std::cout << "Graph file:" << graphFile << "\n";

    pando::GlobalPtr<galois::DistArrayCSR<VertexType, EdgeType>> dGraphPtr =
        static_cast<decltype(dGraphPtr)>(pando::getDefaultMainMemoryResource()->allocate(
            sizeof(galois::DistArrayCSR<VertexType, EdgeType>)));
    pando::Notification isDone;
    PANDO_CHECK(isDone.init());
    wf::ImportGraph<VertexType, EdgeType>(isDone.getHandle(), dGraphPtr, fname, strlen(graphFile));
    isDone.wait();
    if (programOptions.disableKernel2) {
      pando::waitAll();
      return 0;
    }
    std::uint64_t numEpochs = programOptions.epochs;
    runGNN(dGraphPtr, numEpochs);
  }
  pando::waitAll();
  return 0;
}
