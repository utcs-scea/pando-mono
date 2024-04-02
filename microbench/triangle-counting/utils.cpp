// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include "utils.hpp"

// Process command line flags
std::shared_ptr<CommandLineOptions> read_cmd_line_args(int argc, char** argv) {
  std::shared_ptr<CommandLineOptions> opts_ptr = std::make_shared<CommandLineOptions>();

  // Other libraries may have called getopt before, so we reset optind for correctness
  optind = 0;

  int32_t flag = 0;
  int32_t num_vertices = 0;
  while ((flag = getopt(argc, argv, "v:i:lb")) != -1) {
    switch (flag) {
      case 'v':
        sscanf(optarg, "%d", &num_vertices);
        opts_ptr->num_vertices = num_vertices;
        break;
      case 'i':
        opts_ptr->elFile = std::string((const char*)optarg);
        break;
      case 'l':
        opts_ptr->load_balanced_graph = true;
        break;
      case 'b':
        opts_ptr->bsp = true;
        break;
      case '?':
        if (optopt == 'v' || optopt == 'i')
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

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << " -i filepath -v numVertices" << std::endl;
  std::exit(EXIT_FAILURE);
}
