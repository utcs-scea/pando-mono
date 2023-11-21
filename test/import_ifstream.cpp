// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <getopt.h>
#include <pando-rt/export.h>

#include <cstdint>
#include <iostream>

#include <pando-lib-galois/graphs/edge_list_importer.hpp>
#include <pando-lib-galois/import/ifstream.hpp>
#include <pando-rt/memory_resource.hpp>
#include <pando-rt/pando-rt.hpp>

void printUsageExit(char* argv0) {
  std::cerr << "Usage: " << argv0 << "-f filepath" << std::endl;
  std::cerr << "The expected input is a file of integers then two newlines in a row, then it tests "
               "getline"
            << std::endl;
  std::cerr << "The output should be the same as the input file" << std::endl;
  std::exit(EXIT_FAILURE);
}

int pandoMain(int argc, char** argv) {
  char* filepath = nullptr;
  int opt;

  while ((opt = getopt(argc, argv, "f:")) != -1) {
    switch (opt) {
      case 'f':
        filepath = optarg;
        break;
      default:
        printUsageExit(argv[0]);
    }
  }

  if (filepath == nullptr) {
    printUsageExit(argv[0]);
  }

  galois::ifstream inputFileStream;
  const auto status = inputFileStream.open(filepath);
  if (status != pando::Status::Success) {
    printUsageExit(argv[0]);
  }

  // Test reading uint64_t types
  if (pando::getCurrentPlace().node.id == 0) {
    char new0, new1;
    uint64_t val;
    while (inputFileStream.get(new0) && inputFileStream.get(new1) &&
           !(new0 == '\n' && new1 == '\n')) {
      inputFileStream.unget();
      inputFileStream.unget();
      inputFileStream >> val;
      std::cout << val << std::endl;
    }
    std::cout << std::endl;

    char blah[101];
    blah[100] = '\0';
    std::int64_t rdSz;
    while ((rdSz = inputFileStream.getline(blah, 100, '\n')) > 0) {
      if (rdSz > 100) {
        std::cerr << "You read too many characters in this read\n";
        std::exit(EXIT_FAILURE);
      }
      std::cout << blah;
      if (rdSz != 100) {
        std::cout << std::endl;
      }
    }
  }
  return 0;
}
