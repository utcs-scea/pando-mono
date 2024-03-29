// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include "common.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pandohammer/atomic.h>
#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>
#include <unistd.h>

#define __l1sp__ __attribute__((section(".dmem")))
#define __l2sp__ \
  __attribute__((section(".dram"))) // TODO: this is actually l2sp; need fix in linker script

__l2sp__ std::atomic<int64_t> ph_ready;
__l2sp__ std::atomic<int64_t> cp_ready;
__l2sp__ std::atomic<int64_t> ph_done;

// graph data
__l2sp__ vertex_t g_V;
__l2sp__ vertex_t g_E;
__l2sp__ vertex_pointer_t g_fwd_offsets;
__l2sp__ vertex_pointer_t g_fwd_edges;
__l2sp__ vertex_pointer_t g_rev_offsets;
__l2sp__ vertex_pointer_t g_rev_edges;
__l2sp__ vertex_pointer_t g_distance;

__l2sp__ bool g_rev_not_fwd;
__l2sp__ int g_mf;
__l2sp__ int g_mu;

__l2sp__ frontier_data frontier[3];

__l2sp__ barrier_data g_barrier_data;

/**
 * @brief Wait for the CP to complete initialization
 */
int wait_for_cp() {
  // pr_dbg("Telling CP we're ready\n");
  //  let ph know we're ready
  int64_t n = ph_ready.fetch_add(1, std::memory_order_relaxed);
  // pr_dbg("%d-th core ready\n", n);

  // command_processor_ready.store(-1, std::memory_order_relaxed);
  int64_t ready = cp_ready.load(std::memory_order_relaxed);
  int x = 0;
  while (ready != 1) {
    // wait for command processor to be ready
    wait(numPodCores() * myCoreThreads());
    ready = cp_ready.load(std::memory_order_relaxed);
    x++;
  }
  // pr_dbg("Command processor ready\n");
  return 0;
}

int signal_ph_done() {
  ph_done.fetch_add(1, std::memory_order_relaxed);
  return 0;
}

int main() {
  wait_for_cp();
// #define EMPTY_RUN
#ifndef EMPTY_RUN
  barrier_ref barrier = &g_barrier_data;
  barrier.sync([=]() {
    pr_dbg("g_V           = %d\n", g_V);
    pr_dbg("g_E           = %d\n", g_E);
    pr_dbg("g_fwd_offsets = %lx\n", (unsigned long)g_fwd_offsets);
    pr_dbg("g_fwd_edges   = %lx\n", (unsigned long)g_fwd_edges);
    pr_dbg("g_rev_offsets = %lx\n", (unsigned long)g_rev_offsets);
    pr_dbg("g_rev_edges   = %lx\n", (unsigned long)g_rev_edges);
    pr_dbg("g_distance    = %lx\n", (unsigned long)g_distance);
    pr_dbg("threads = %d, cores = %d, threads_per_core = %d\n", threads(), numPodCores(),
           myCoreThreads());
  });
  // breadth first search
  vertex_pointer_t l_distance = g_distance;
  vertex_pointer_t l_fwd_offsets = g_fwd_offsets;
  vertex_pointer_t l_fwd_edges = g_fwd_edges;
  vertex_pointer_t l_rev_offsets = g_rev_offsets;
  vertex_pointer_t l_rev_edges = g_rev_edges;
  vertex_t iter = 0;

  frontier_ref curr_frontier = &frontier[0];
  frontier_ref next_frontier = &frontier[1];
  frontier_ref tmp_frontier = &frontier[2];

  while (curr_frontier.size() != 0) {
    vertex_t distance = iter + 1;
    barrier.sync([=]() {
      g_mu = 0;
      g_mf = 0;
    });

    ////////////////////////////////////////////
    // decide the direction of this iteration //
    ////////////////////////////////////////////
    if (!g_rev_not_fwd) {
      // make frontier sparse if needed
      curr_frontier = curr_frontier.to_sparse(tmp_frontier, barrier, g_V);

      // do we switch from forward to reverse?
      // 1. find sum (degree in frontier)
      int mf_local = 0;
      for (int src_i = my_thread(); src_i < curr_frontier.size(); src_i += threads()) {
        // todo: hoist reading the pointer vertices out of this loop
        int src = curr_frontier.vertices(src_i);
        mf_local += l_fwd_offsets[src + 1] - l_fwd_offsets[src];
      }
      atomic_fetch_add_i32(&g_mf, mf_local);
      // 2. find sum (degree unvisited)
      int mu_local = 0;
      for (int v = my_thread(); v < g_V; v += threads()) {
        if (l_distance[v] == -1) {
          mu_local += l_fwd_offsets[v + 1] - l_fwd_offsets[v];
        }
      }
      atomic_fetch_add_i32(&g_mu, mu_local);
      barrier.sync([=]() {
        g_rev_not_fwd = (g_mf > (g_mu / 20));
        // std::cout << "Iteration " << std::setw(2) << iter
        //           << ": " << std::setw(3) << g_mf << " mf, "
        //           << std::setw(3) << g_mu << " mu, "
        //           << (g_rev_not_fwd ? "rev" : "fwd") << std::endl;
      });
    } else {
      // do we switch from reverse to forward?
      // printf("Thread %d: deciding to switch back\n", my_thread());
      barrier.sync([=]() {
        g_rev_not_fwd = (curr_frontier.size() >= (g_V / 20));
        // std::cout << "Iteration " << std::setw(2) << iter
        //           << ": curr_frontier.size()=" << std::setw(3) << curr_frontier.size()
        //           << ", (V/20) = (" << V << "/20) = " << (V/20) << ", "
        //           << (g_rev_not_fwd ? "rev" : "fwd") << std::endl;
      });
    }
    barrier.sync();

    if (g_rev_not_fwd) {
      barrier.sync([=]() {
        pr_dbg("iteration %d: curr_frontier size = %d\n", iter, curr_frontier.size());
        pr_dbg("curr_frontier is sparse\n");
      });
      // make frontier dense if needed
      curr_frontier = curr_frontier.to_dense(tmp_frontier, barrier, g_V);
      // traverse backards
      vertex_t contrib = 0;
      for (int dst = my_thread(); dst < g_V; dst += threads()) {
        if (l_distance[dst] == -1) {
          vertex_t src_start = l_rev_offsets[dst];
          vertex_t src_stop = l_rev_offsets[dst + 1];
          for (vertex_t src_i = src_start; src_i < src_stop; src_i++) {
            vertex_t src = l_rev_edges[src_i];
            if (curr_frontier.vertices(src) == 1) {
              l_distance[dst] = distance;
              next_frontier.vertices(dst) = 1;
              contrib++;
              break;
            }
          }
        }
      }
      atomic_fetch_add_i64(&next_frontier.size(), contrib);

      // wait for all threads to finish
      barrier.sync();

      // swap frontiers
      swap(curr_frontier, next_frontier);
      next_frontier.clear(barrier, g_V);

    } else {
      // make frontier sparse if needed
      curr_frontier = curr_frontier.to_sparse(tmp_frontier, barrier, g_V);
      barrier.sync([=]() {
        pr_dbg("iteration %d: curr_frontier size = %d\n", iter, curr_frontier.size());
        pr_dbg("curr_frontier is sparse\n");
      });
      vertex_t size = curr_frontier.size();
      vertex_t contrib = 0;
      for (vertex_t src_i = my_thread(); src_i < size; src_i += threads()) {
        // for (vertex_t src_i = 0; src_i < size; src_i++) {
        vertex_t src = curr_frontier.vertices(src_i);
        vertex_t start = l_fwd_offsets[src];
        vertex_t end = l_fwd_offsets[src + 1];
        for (vertex_t edge_i = start; edge_i < end; edge_i++) {
          vertex_t dst = l_fwd_edges[edge_i];
          // cas here when available
          if (l_distance[dst] == -1) {
            if (atomic_swap_i32(&l_distance[dst], distance) == -1) {
              next_frontier.vertices(dst) = 1;
              contrib++;
            }
          }
        }
      }
      atomic_fetch_add_i64(&next_frontier.size(), contrib);
      barrier.sync();
      swap(curr_frontier, next_frontier);
      next_frontier.clear(barrier, g_V);
    }
    iter++;
  }

  // pr_dbg("PH: Done (curr_frontier.size() = %d\n"
  //,curr_frontier.size()
  //);
#endif
  signal_ph_done();
  return 0;
}
