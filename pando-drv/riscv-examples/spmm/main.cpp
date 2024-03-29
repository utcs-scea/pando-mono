// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
/* Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. */

#include <algorithm>
#include <ostream>
#include <tuple>
#include <vector>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>

std::vector<std::vector<float>> features;
std::vector<std::vector<float>> output;

struct coord {
  int x;
  int y;
};

template <typename ValueType, typename OffsetT = ptrdiff_t>
struct CountingIterator {
  typedef CountingIterator self_type;
  typedef OffsetT difference_type;
  typedef ValueType value_type;
  typedef ValueType* pointer;
  typedef ValueType reference;
  typedef std::random_access_iterator_tag iterator_category;

  ValueType val;

  inline CountingIterator(const ValueType& val) : val(val) {}

  template <typename Distance>
  inline reference operator[](Distance n) const {
    return val + n;
  }

  friend std::ostream& operator<<(std::ostream& os, const self_type& itr) {
    os << "[" << itr.val << "]";
    return os;
  }
};

class CSRMatrix {
public:
  std::vector<float> values;
  std::vector<int> colIndices;
  std::vector<int> rowOffsets;
  int numRows;
  CSRMatrix(int num_rows, int num_edges, std::vector<int>& coo_row_indices,
            std::vector<int>& coo_col_indices)
      : numRows(num_rows) {
    values.resize(num_edges, 1.0);
    colIndices.resize(num_edges, 0);

    // creating a vector of tuples row and column indices
    std::vector<std::tuple<int, int>> coo_tuples;
    for (int i = 0; i < coo_row_indices.size(); i++) {
      coo_tuples.push_back(std::make_tuple(coo_row_indices[i], coo_col_indices[i]));
    }

    // sorting the tuples based on row indices
    std::sort(coo_tuples.begin(), coo_tuples.end(),
              [](std::tuple<int, int> a, std::tuple<int, int> b) {
                return std::get<0>(a) < std::get<0>(b);
              });

    for (int i = 0; i < coo_tuples.size(); i++) {
      coo_row_indices[i] = std::get<0>(coo_tuples[i]);
      coo_col_indices[i] = std::get<1>(coo_tuples[i]);
    }

    // moving column indices
    colIndices = std::move(coo_col_indices);

    // creating row offsets
    int prev_node = -1;
    int count = 0;
    for (auto index : coo_row_indices) {
      if (index != prev_node) {
        assert(index > prev_node);
        for (int i = 0; i < index - prev_node; i++) {
          rowOffsets.push_back(count);
        }
        prev_node = index;
      }
      count++;
    }
    rowOffsets.push_back(count);
  }
};

int readMtx(const char* fname, int& num_rows, int& num_cols, int& num_nzs,
            std::vector<int>& row_indices, std::vector<int>& col_indices) {
  FILE* fp_ = fopen(fname, "r");
  printf("Reading file '%s'\n", fname);
  if (!fp_) {
    fprintf(stdout, "Error: failed to open '%s': %s\n", fname, strerror(errno));
    exit(1);
  }

  int r = fscanf(fp_, "%d %d %d", &num_rows, &num_cols, &num_nzs);
  if (r != 3) {
    fprintf(stdout, "Error: failed to open '%s': %s\n", fname, strerror(errno));
    fclose(fp_);
    exit(1);
  }

  row_indices.resize(num_nzs, 0);
  col_indices.resize(num_nzs, 0);

  for (int i = 0; i < num_nzs; i++) {
    int src, dest;
    int r = fscanf(fp_, "%d %d", &src, &dest);
    row_indices[i] = src;
    col_indices[i] = dest;
    if (r != 2) {
      fprintf(stderr, "Error: unexpected end of file for '%s': %m\n", fname);
      fclose(fp_);
      return -1;
    }
  }
  return 0;
}

int readFeatures(const char* fname, std::vector<std::vector<float>>& features) {
  FILE* fp_ = fopen(fname, "r");
  printf("Reading file '%s'\n", fname);
  if (!fp_) {
    fprintf(stderr, "Error: failed to open '%s': %m\n", fname);
    return -1;
  }

  int num_rows, num_cols;
  int r = fscanf(fp_, "%d %d", &num_rows, &num_cols);
  if (r != 2) {
    fprintf(stderr, "Error: unexpected end of file for '%s': %m\n", fname);
    fclose(fp_);
    return -1;
  }

  features.resize(num_rows);

  for (int i = 0; i < num_rows; i++) {
    features[i].resize(num_cols, (float)0.0);
    float value;
    for (int j = 0; j < num_cols; j++) {
      int r = fscanf(fp_, "%f", &value);
      if (r != 1) {
        fprintf(stderr, "Error: unexpected end of file for '%s': %m\n", fname);
        fclose(fp_);
        return -1;
      }
      features[i][j] = value;
    }
  }
  return 0;
}

void SpMMValidation(CSRMatrix& graph, std::vector<std::vector<float>>& features,
                    std::vector<std::vector<float>>& output) {
#pragma unroll
  for (int row = 0; row < graph.rowOffsets.size() - 1; row++) {
    //        for (int col = 0; col < features[0].size(); col++) {
    int col = 0;
    {
      float nonzero = 0;
      for (int offset = graph.rowOffsets[row]; offset < graph.rowOffsets[row + 1]; offset++) {
        nonzero += graph.values[offset] * features[graph.colIndices[offset]][col];
        printf("row: %d col: %d offset: %d nonzero: %d\n", row, col, offset, nonzero);
      }
      //            ph_print_float(nonzero);
      ph_print_int(row);
      ph_print_int(col);
      //            printf("row: %d col: %d base: %p pointer: %p\n", row, col, &output[0][0],
      //            &output[row][col]);
      ph_print_hex((uint64_t)&output[row][col]);
      if (nonzero != output[row][col]) {
        //                printf("!row: %d col: %d nonzero: %d\n",
        //                        row , col , nonzero);
        throw std::runtime_error("Invalid output matrix");
      }
    }
  }
}

void SpMMPrint(CSRMatrix& graph, std::vector<std::vector<float>>& features,
               std::vector<std::vector<float>>& output) {
#pragma unroll
  for (int row = 0; row < graph.rowOffsets.size() - 1; row++) {
    //        for (int col = 0; col < features[0].size(); col++) {
    int col = 0;
    {
      float nonzero = 0;
      for (int offset = graph.rowOffsets[row]; offset < graph.rowOffsets[row + 1]; offset++) {
        nonzero += graph.values[offset] * features[graph.colIndices[offset]][col];
        printf("row: %d col: %d offset: %d nonzero: %d\n", row, col, offset, nonzero);
      }
    }
  }
}

void CoordinatesSearch(int diagonal, int* a, CountingIterator<int> b, int a_len, int b_len,
                       coord& th_coord) {
  int x_min = (diagonal - b_len) > 0 ? (diagonal - b_len) : 0;
  int x_max = diagonal < a_len ? diagonal : a_len;

  while (x_min < x_max) {
    int x_pivot = (x_min + x_max) >> 1;
    if (a[x_pivot] <= b[diagonal - x_pivot - 1]) {
      x_min = x_pivot + 1;
    } else {
      x_max = x_pivot;
    }
  }

  th_coord.x = std::min(x_min, a_len);
  th_coord.y = diagonal - x_min;
}

void MergePathSpMM(int tid, int num_threads, CSRMatrix& graph,
                   std::vector<std::vector<float>>& features,
                   std::vector<std::vector<float>>& output) {
  printf("Thread %d, %d thread(s)\n", tid, num_threads);

  CountingIterator<int> nz_indices(0);

  int num_merge_items = graph.numRows + graph.values.size();
  int items_per_thread = (num_merge_items + num_threads - 1) / num_threads;

  coord thread;
  coord thread_end;
  int start_diagonal = std::min(items_per_thread * tid, num_merge_items);
  int end_diagonal = std::min(start_diagonal + items_per_thread, num_merge_items);

  CoordinatesSearch(start_diagonal, &graph.rowOffsets[1], nz_indices, graph.rowOffsets.size() - 1,
                    graph.colIndices.size(), thread);
  CoordinatesSearch(end_diagonal, &graph.rowOffsets[1], nz_indices, graph.rowOffsets.size() - 1,
                    graph.colIndices.size(), thread_end);

  printf("num_merge_items: %d\n", num_merge_items);
  printf("items_per_thread: %d\n", items_per_thread);
  printf("start_diagonal: %d\n", start_diagonal);
  printf("end_diagonal: %d\n", end_diagonal);
  printf("thread.x: %d\n", thread.x);
  printf("thread.y: %d\n", thread.y);
  printf("thread_end.x: %d\n", thread_end.x);
  printf("thread_end.y: %d\n", thread_end.y);

  for (int col = 0; col < features[0].size(); col++) {
    int x = thread.x;
    int y = thread.y;
    for (; x < thread_end.x; ++x) {
      float nonzero = 0;
      for (; y < graph.rowOffsets[x + 1]; ++y) {
        nonzero += graph.values[y] * features[graph.colIndices[y]][col];
        printf("1 row: %d col: %d offset: %d nonzero: %f\n", x, col, y, nonzero);
      }

      //            std::atomic_ref<float> ref(output[x][col]);
      //            ref.fetch_add(nonzero);
      output[x][col] += nonzero;

      nonzero = 0;
      for (; y < thread_end.y; y++) {
        nonzero += graph.values[y] * features[graph.colIndices[y]][col];
        printf("1 row: %d col: %d offset: %d nonzero: %f\n", thread_end.x, col, y, nonzero);
      }

      if (nonzero != 0) {
        //                std::atomic_ref<float> ref2(output[thread_end.x][col]);
        //                ref2.fetch_add(nonzero);
        output[thread_end.x][col] += nonzero;
      }
    }
  }
}

int main(int argc, const char* argv[]) {
  uint64_t tid = (myCoreId() << 4) + myThreadId();

  if (tid != 0)
    return 0;

  ph_puts("SpMM\n");
  ph_puts("Reading the graph\n");
  int numRows, numCols, numNzs;
  std::vector<int> rowIndices;
  std::vector<int> colIndices;
  readMtx("graph.mtx", numRows, numCols, numNzs, rowIndices, colIndices);

  ph_puts("Reading the features\n");
  readFeatures("features", features);

  ph_puts("Constructing CSR\n");
  CSRMatrix graph(numRows, numNzs, rowIndices, colIndices);

  printf("values %d\n", graph.values.size());
  for (auto& elem : graph.values) {
    ph_print_float(elem);
  }
  printf("\n");
  printf("colIndices %d\n", graph.colIndices.size());
  for (auto& elem : graph.colIndices) {
    printf("%d ", elem);
  }
  printf("\n");
  printf("rowOffsets %d\n", graph.rowOffsets.size());
  for (auto& elem : graph.rowOffsets) {
    printf("%d ", elem);
  }
  printf("\n");

  //    printf("Output matrix %d %d\n", graph.numRows, features[0].size());
  //    output.resize(graph.numRows);
  //    for (auto &col: output) {
  //        col.resize(features[0].size(), (float)0.0);
  //    }

  ph_puts("MergePath SpMM\n");
  int num_threads = 3;
  //    SpMMPrint(graph, features, output);
  MergePathSpMM(0, num_threads, graph, features, output);
  MergePathSpMM(1, num_threads, graph, features, output);
  MergePathSpMM(2, num_threads, graph, features, output);

  //    ph_puts("Validation\n");
  //    SpMMValidation(graph, features, output);

  //    printf("Out matrix\n");
  //    for (auto &row: output) {
  //        for (auto &elem: row) {
  ////            printf("%d ", elem);
  //            ph_print_int(elem);
  //        }
  ////        printf("\n");
  //    }

  printf("Done\n");
  return 0;
}
