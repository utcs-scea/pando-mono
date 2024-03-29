// SPDX-License-Identifier: MIT

#ifndef __AGILE_MODELING_DRV_EXAMPLES_APP_MAIN_INPUTS_BINARYARR_H_
#define __AGILE_MODELING_DRV_EXAMPLES_APP_MAIN_INPUTS_BINARYARR_H_

#include <cinttypes>
#include <tuple>
#include <filesystem>

struct Vertex {
  uint64_t id;        // GlobalIDS: global id ... Vertices: vertex id
  uint64_t edges;     // number of edges
  uint64_t start;     // start index in compressed edge list
  int    type;
};

struct Edge {
  uint64_t src;     // vertex id of src
  uint64_t dst;     // vertex id of dst
  int    type;
  int    src_type;
  int    dst_type;
  uint64_t src_glbid;
  uint64_t dst_glbid;
};


#endif // __AGILE_MODELING_DRV_EXAMPLES_APP_MAIN_INPUTS_BINARYARR_H_
