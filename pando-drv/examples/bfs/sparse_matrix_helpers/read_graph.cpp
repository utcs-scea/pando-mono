// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "read_graph.hpp"
#include "mmio.h"
#include <vector>
#include <algorithm>
#include <stdio.h>

/**
 * reference the column field of nonzero
 */
static int &column(int &i) { return i; }
static int &column(std::pair<int, float> &pair) { return pair.first; }

/**
 * transpose nonzero (int)
 */
static
void transpose_nonzero(int &row, int &nz)
{
    int tmp = row;
    row = nz;
    nz = tmp;
}

/**
 * transpose nonzero (pair)
 */
static
void transpose_nonzero(int &row, std::pair<int, float> &pair)
{
    int tmp = row;
    row = pair.first;
    pair.first = tmp;
}

/**
 * scan a nonzero from a file (int)
 */
static
int read_scan_nonzero(const std::string &fname, FILE *f, const MM_typecode& banner, int &row, int &nz) {
    int s, d, vi;
    float vd;
    int r = 0;
    if (mm_is_real(banner)) {
        r = fscanf(f, "%d %d %f", &s, &d, &vd);
    } else if (mm_is_integer(banner)) {
        r = fscanf(f, "%d %d %d", &s, &d, &vi);
    } else if (mm_is_pattern(banner)) {
        r = fscanf(f, "%d %d", &s, &d);
        r++;
    }
    if (r != 3) {
        fprintf(stderr, "Error: unexpected end of file for '%s': %m\n", fname.c_str());
        fclose(f);
        return -1;
    }
    row = s;
    nz  = d;
    return 0;
}
/**
 * scan a nonzero from a file (pair)
 */
static
int read_scan_nonzero(const std::string &fname, FILE *f, const MM_typecode& banner, int &row, std::pair<int, float>&nz) {
    int s, d, vi;
    float vd;
    int r = 0;
    if (mm_is_real(banner)) {
        r = fscanf(f, "%d %d %f", &s, &d, &vd);
    } else if (mm_is_integer(banner))  {
        r = fscanf(f, "%d %d %d", &s, &d, &vi);
    } else if (mm_is_pattern(banner)) {
        r = fscanf(f, "%d %d", &s, &d);
        vd = 1.0;
        r++;
    }
    if (r != 3) {
        fprintf(stderr, "Error: unexpected end of file for '%s': %m\n", fname.c_str());
        fclose(f);
        return -1;
    }
    row = s;
    nz.first = d;
    nz.second = vd;
    return 0;
}
/**
 * sort row of nonzeros
 */
static void read_sort_row(std::vector<int>&v) {std::sort(v.begin(), v.end());}
static void read_sort_row(std::vector<std::pair<int,float>>&v) {
    std::sort(v.begin(), v.end(), [](const std::pair<int, float>&lhs,
                                     const std::pair<int, float>&rhs) {
                  return lhs.first < rhs.first;
              });
}

/**
 * shared read implementation
 */
template <typename NonZeroT>
static
int read_common(
    const std::string &graph,
    /* outputs */
    int *V,
    int *E,
    std::vector<int> &offsets,
    std::vector<NonZeroT> &nonzeros){
    // 1. open the file
    printf("Reading file '%s'\n", graph.c_str());
    FILE *f = fopen(graph.c_str(), "r");
    if (!f) {
        fprintf(stderr, "Error: failed to open '%s': %m\n", graph.c_str());
        return -1;
    }
    // 2. read the banner and matrix info
    MM_typecode banner;
    int r = mm_read_banner(f, &banner);
    if (r != 0) {
        fprintf(stderr, "Error: failed to read banner from '%s': %m\n", graph.c_str());
        return -1;
    }

    if (!mm_is_sparse(banner) ||
        !(mm_is_real(banner) || mm_is_integer(banner) || mm_is_pattern(banner)) ||
        !(mm_is_general(banner) || mm_is_symmetric(banner))) {
        fclose(f);
        fprintf(stderr, "Unsupported graph input\n");
        return -1;
    }

    int M, N, nz;
    r = mm_read_mtx_crd_size(f, &M, &N, &nz);
    if (r != 0) {
        fclose(f);
        fprintf(stderr, "Error: failed to matrix crd size from '%s': %m\n", graph.c_str());
        return -1;
    }
    *V = M;
    *E = nz;
    if (mm_is_symmetric(banner))
        *E *= 2;

    printf("Reading graph '%s': V = %d, E = %d\n", graph.c_str(), *V, *E);

    // 3. read the nonzeros
    std::vector<std::vector<NonZeroT>> rows((*V)+1);
    for (int i = 0; i < nz; i++) {
        NonZeroT nonzero {};
        int row = 0;
        r = read_scan_nonzero(graph, f, banner, row, nonzero);
        // matrix market is 1 indexed, but we use zero indexing
        row--;
        column(nonzero)--;
        // add transpose nonzero to the row
        rows[row].push_back(nonzero);
        // matrix is symmetric, we need to add the transpose nonzero
        if (mm_is_symmetric(banner)) {
            transpose_nonzero(row, nonzero);
            rows[row].push_back(nonzero);
        }
    }
    fclose(f);
    printf("Converting to CSR\n");
    // 4. conver to csr
    offsets = std::vector<int>((*V)+1);
    nonzeros = std::vector<NonZeroT>(*E);
    int sum = 0;
    for (size_t i = 0; i < offsets.size(); i++) {
        offsets[i] = sum;
        sum += rows[i].size();
    }

    int n = 0;
    for (int i = 0; i < *V; i++) {
        read_sort_row(rows[i]);
        //std::sort(rows[i].begin(), rows[i].end());
        for (NonZeroT &j : rows[i]) {
            nonzeros[n++] = j;
        }
    }
    return 0;
}

int read_sparse_matrix(
    const std::string &sparse_matrix,
    /* output */
    int *V,
    int *E,
    std::vector<int> &offsets,
    std::vector<std::pair<int, float>> & nonzeros) {
    return read_common<std::pair<int,float>>(sparse_matrix, V, E, offsets, nonzeros);
}

int read_graph(
    const std::string &graph,
    /* outputs */
    int *V,
    int *E,
    std::vector<int> &offsets,
    std::vector<int> &nonzeros){
    return read_common<int>(graph, V, E, offsets, nonzeros);
}
