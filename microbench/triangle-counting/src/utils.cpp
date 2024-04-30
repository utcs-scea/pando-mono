// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <utils.hpp>

// Process command line flags
std::shared_ptr<CommandLineOptions> read_cmd_line_args(int argc, char** argv) {
  std::shared_ptr<CommandLineOptions> opts_ptr = std::make_shared<CommandLineOptions>();

  // Other libraries may have called getopt before, so we reset optind for correctness
  optind = 0;

  int32_t flag = 0;
  int32_t num_vertices = 0;
  int32_t tc_chunk = NO_CHUNK;
  int32_t graph_type = DLCSR;
  while ((flag = getopt(argc, argv, "v:i:c:g:")) != -1) {
    switch (flag) {
      case 'v':
        sscanf(optarg, "%d", &num_vertices);
        opts_ptr->num_vertices = num_vertices;
        break;
      case 'i':
        opts_ptr->elFile = std::string((const char*)optarg);
        break;
      case 'g':
        sscanf(optarg, "%d", &graph_type);
        switch (graph_type) {
          case 0:
            opts_ptr->graph_type = DLCSR;
            break;
          case 1:
            opts_ptr->graph_type = MDLCSR;
            break;
          case 2:
            opts_ptr->graph_type = DACSR;
            break;
          default:
            printUsageExit(argv[0]);
        }
        break;
      case 'c':
        sscanf(optarg, "%d", &tc_chunk);
        switch (tc_chunk) {
          case 0:
            opts_ptr->tc_chunk = NO_CHUNK;
            break;
          case 1:
            opts_ptr->tc_chunk = CHUNK_VERTICES;
            break;
          case 2:
            opts_ptr->tc_chunk = CHUNK_EDGES;
            break;
          default:
            printUsageExit(argv[0]);
        }
        break;
      case 'h':
        printUsage(argv[0]);
        std::exit(0);
      case '?':
        if (optopt == 'v' || optopt == 'i' || optopt == 'c' || optopt == 'g')
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
          fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
        return nullptr;
      default:
        printUsageExit(argv[0]);
    }
  }
  if (opts_ptr->elFile == "" || opts_ptr->num_vertices == 0)
    printUsageExit(argv[0]);
  return opts_ptr;
}

void printUsage(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -i filepath -v numVertices" << std::endl;
  std::cerr << "Can specify runtime algorithm with -c. Valid options: [0 (NO_CHUNK), 1 "
               "(CHUNK_EDGES), 2 (CHUNK_VERTICES)]\n";
  std::cerr << "\n Can specify graph_type with -g. Valid options: [0 (DistLocalCSR), 1 "
               "(MirroredDistLocalCSR), 2 (DistArrayCSR))]\n";
}

void printUsageExit(char* argv0) {
  printUsage(argv0);
  std::exit(EXIT_FAILURE);
}
