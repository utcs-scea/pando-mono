// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>

#include "pando-rt/export.h"
#include "pando-rt/pando-rt.hpp"
#include "pando-rt/sync/notification.hpp"

#include "pando-wf1/gnn.hpp"
#include "pando-wf1/graphs/graphtypes.hpp"

#include <pando-lib-galois/graphs/dist_local_csr.hpp>
#include <pando-lib-galois/import/ingest_wmd_csv.hpp>
#include <pando-lib-galois/loops/do_all.hpp>

using VertexType = galois::VertexEmbedding;
using EdgeType = galois::WMDEdge;
using Graph = galois::DistLocalCSR<VertexType, EdgeType>;

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

void runGNN(pando::GlobalPtr<Graph> dGraphPtr, std::uint64_t numEpochs) {
  Graph g = *dGraphPtr;
  std::cout << "[Starts GNN vertex classification environment setup]\n";
  gnn::GraphNeuralNetwork<Graph> gnn;
  gnn.initialize(dGraphPtr);
  std::cout << "[Starts GNN vertex classification epochs (Epochs:" << numEpochs << ")\n";
  gnn.Train(numEpochs);
  std::cout << "[Completes GNN vertex classification epochs (Epochs:" << numEpochs << ")\n";
}

int pandoMain(int argc, char** argv) {
  const auto thisPlace = pando::getCurrentPlace();
  ProgramOptions programOptions;
  programOptions.Parse(argc, argv);
  if (thisPlace.node.id == 0) {
    const char* graphFile = programOptions.graphFile;
    pando::GlobalPtr<char> fname = static_cast<pando::GlobalPtr<char>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(char) * strlen(graphFile)));
    for (size_t i = 0; i < strlen(graphFile); ++i) {
      fname[i] = graphFile[i];
    }
    pando::Array<char> filename;
    std::string strGraphFile = std::string(graphFile);
    PANDO_CHECK(filename.initialize(strGraphFile.size()));

    for (size_t i = 0; i < strGraphFile.size(); ++i)
      filename[i] = strGraphFile[i];

    std::cout << "[Graph File Path: " << graphFile << " ..]\n";

    std::cout << "[Starts graph construction]\n";
    pando::GlobalPtr<Graph> dGraphPtr = static_cast<decltype(dGraphPtr)>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(Graph)));
    *dGraphPtr = galois::initializeWMDDLCSR<VertexType, EdgeType>(filename);
    std::cout << "[Completes graph construction]\n";
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
