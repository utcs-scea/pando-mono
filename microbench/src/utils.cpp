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
  int32_t rt_algo = BASIC;
  while ((flag = getopt(argc, argv, "v:i:a:l")) != -1) {
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
      case 'a':
        sscanf(optarg, "%d", &rt_algo);
        switch (rt_algo) {
          case 0:
            opts_ptr->rt_algo = BASIC;
            break;
          case 1:
            opts_ptr->rt_algo = BASP;
            break;
          case 2:
            opts_ptr->rt_algo = BSP;
            break;
          default:
            printUsageExit(argv[0]);
        }
        break;
      case 'h':
        printUsage(argv[0]);
        std::exit(0);
      case '?':
        if (optopt == 'v' || optopt == 'i' || optopt == 'a')
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
  std::cerr
      << "\n Can specify runtime algorithm with -a. Valid options: [0 (ASP), 1 (BASP), 2 (BSP)]\n";
}

void printUsageExit(char* argv0) {
  printUsage(argv0);
  std::exit(EXIT_FAILURE);
}
