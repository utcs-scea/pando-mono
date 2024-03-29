// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "breadth_first_search_graph.hpp"
#include "read_graph.hpp"
#include "transpose_graph.hpp"
#include <DrvAPI.hpp>
#include <iomanip>
#include <iostream>
#include <string>

#define THREADS_PER_CORE (DrvAPI::myCoreThreads())
#define CORES            (DrvAPI::numPodCores())
#define THREADS          (THREADS_PER_CORE * CORES)

struct barrier_data {
  int count;
  int signal;
  int sense;
};

using namespace DrvAPI;

static int my_thread() {
  return myThreadId() + myCoreId() * THREADS_PER_CORE;
}

// creates the barrier_data_ref class
DRV_API_REF_CLASS_BEGIN(barrier_data)
DRV_API_REF_CLASS_DATA_MEMBER(barrier_data, count)
DRV_API_REF_CLASS_DATA_MEMBER(barrier_data, signal)
DRV_API_REF_CLASS_DATA_MEMBER(barrier_data, sense)
void sync() {
  sync([]() {
  });
}
template <typename F>
void sync(F f) {
  int signal_ = signal();
  int count_ = DrvAPI::atomic_add(&count(), 1);
  // printf("Thread %d: count = %d\n", my_thread(), count_);
  if (count_ == THREADS - 1) {
    // printf("Thread %d: witing signal = %d\n", my_thread(), signal_);
    count() = 0;
    f();
    signal() = !signal_;
  } else {
    static constexpr int backoff_limit = 1000;
    int backoff_counter = 8;
    while (signal() == signal_) {
      DrvAPI::wait(backoff_counter);
      backoff_counter = std::min(backoff_counter * 2, backoff_limit);
    }
    // printf("Thread %d: received signal = %d\n", my_thread(), !signal_);
  }
}

DRV_API_REF_CLASS_END(barrier_data)
using barrier_ref = barrier_data_ref;

template <typename T>
using Global = DrvAPIGlobalL2SP<T>;

template <typename T>
using Pointer = DrvAPIPointer<T>;

Global<barrier_data> g_barrier_data;

// graph data
Global<int> g_V;
Global<int> g_E;
Global<Pointer<int>> g_fwd_offsets;
Global<Pointer<int>> g_fwd_edges;
Global<Pointer<int>> g_rev_offsets;
Global<Pointer<int>> g_rev_edges;
Global<Pointer<int>> g_distance;
Global<bool> g_rev_not_fwd;
Global<int> g_mf;
Global<int> g_mu;

// frontier data
struct frontier_data {
  int size;
  bool is_dense;
  Pointer<int> vertices;
};

DRV_API_REF_CLASS_BEGIN(frontier_data)
DRV_API_REF_CLASS_DATA_MEMBER(frontier_data, size)
DRV_API_REF_CLASS_DATA_MEMBER(frontier_data, vertices)
DRV_API_REF_CLASS_DATA_MEMBER(frontier_data, is_dense)
Pointer<int>::value_handle vertices(int i) {
  Pointer<int> v = vertices();
  return v[i];
}
frontier_data_ref to_sparse(frontier_data_ref& tmp_frontier, barrier_ref barrier) {
  if (is_dense()) {
    barrier.sync([=]() {
      // std::cout << "Thread " << my_thread() << ": converting to sparse" << std::endl;
      tmp_frontier.size() = 0;
      tmp_frontier.is_dense() = false;
    });
    // insert sparse data into output frontier
    int V = g_V;
    for (int v = my_thread(); v < V; v += THREADS) {
      if (vertices(v) == 1) {
        // std::cout << "Thread " << my_thread() << ": adding vertex " << v << std::endl;
        int i = DrvAPI::atomic_add(&tmp_frontier.size(), 1);
        tmp_frontier.vertices(i) = v;
      }
    }
    barrier.sync();
    frontier_data_ref tmp = tmp_frontier;
    tmp_frontier = *this;
    return tmp;
  }
  return *this;
}

frontier_data_ref to_dense(frontier_data_ref& tmp_frontier, barrier_ref barrier) {
  if (!is_dense()) {
    // zero output frontier if needed
    int V = g_V;
    if (tmp_frontier.size() != 0) {
      for (int v = my_thread(); v < V; v += THREADS) {
        tmp_frontier.vertices(v) = 0;
      }
    }
    barrier.sync([=]() {
      tmp_frontier.size() = size();
      tmp_frontier.is_dense() = true;
    });
    // insert sparse data into output frontier
    for (int v_i = my_thread(); v_i < size(); v_i += THREADS) {
      int v = vertices(v_i);
      tmp_frontier.vertices(v) = 1;
    }
    barrier.sync();
    frontier_data_ref tmp = tmp_frontier;
    tmp_frontier = *this;
    return tmp;
  }
  return *this;
}

void clear(barrier_ref barrier) {
  barrier.sync([=]() {
    // std::cout << "Thread " << my_thread() << ": clearing frontier" << std::endl;
    // print_async();
  });
  int V = g_V;
  for (int v = my_thread(); v < V; v += THREADS) {
    vertices(v) = 0;
  }
  barrier.sync([=]() {
    size() = 0;
    is_dense() = true;
    // std::cout << "Thread " << my_thread() << ": cleared frontier" << std::endl;
    // print_async();
  });
}

void print_async() {
  std::cout << "Thread " << my_thread() << ": printing frontier "
            << "@ " << std::hex << std::setw(8) << std::setfill('0') << ptr_ << ", "
            << "size = " << size() << ", "
            << "is_dense = " << is_dense() << ", "
            << "vertices = " << std::hex << std::setw(8) << std::setfill('0')
            << static_cast<Pointer<int>>(vertices()) << ", " << std::dec << std::endl;
}
void print(barrier_ref barrier) {
  barrier.sync([=]() {
    print_async();
  });
}

DRV_API_REF_CLASS_END(frontier_data)
using frontier_ref = frontier_data_ref;
static void swap(frontier_ref& a, frontier_ref& b) {
  frontier_data_ref tmp = a;
  a = b;
  b = tmp;
}

std::array<Global<frontier_data>, 3> g_frontier;

std::vector<int> fwd_offsets;
std::vector<int> fwd_nonzeros;
std::vector<int> rev_offsets;
std::vector<int> rev_nonzeros;
std::vector<int> distance;

Global<int> root_vertex;

// mutex data
struct mutex_data {
  int lock;
};
DRV_API_REF_CLASS_BEGIN(mutex_data)
DRV_API_REF_CLASS_DATA_MEMBER(mutex_data, lock)
void acquire() {
  while (DrvAPI::atomic_swap<int>(&lock(), 1) != 0)
    ;
}
void release() {
  lock() = 0;
}
template <typename F>
void sync(F f) {
  acquire();
  f();
  release();
}
DRV_API_REF_CLASS_END(mutex_data)
using mutex_ref = mutex_data_ref;

Global<mutex_data> g_mutex;

int BFSMain(int argc, char* argv[]) {
  std::string graph_file = "";
  std::string root_vertex_str = "0";
  int arg = 1;

  barrier_ref barrier = &g_barrier_data;
  frontier_ref curr_frontier = &g_frontier[0];
  frontier_ref next_frontier = &g_frontier[1];
  frontier_ref tmp_frontier = &g_frontier[2];
  mutex_ref mutex = &g_mutex;
  // read in graph from file
  if (my_thread() == 0) {
    DrvAPIMemoryAllocatorInit();

    if (arg < argc) {
      graph_file = argv[arg++];
    }

    if (arg < argc) {
      root_vertex_str = argv[arg++];
    }

    if (graph_file == "") {
      // std::cout << "Usage: " << std::string(argv[0]) << " <graph_file> [root_vertex (default 0)]"
      // << std::endl;
      barrier.sync();
      return 1;
    }

    int vertex = std::stoi(root_vertex_str);
    root_vertex = vertex;

    // std::cout << "graph_file = " << graph_file << ", root_vertex = " << vertex << std::endl;
    int V, E;
    read_graph(graph_file, &V, &E, fwd_offsets, fwd_nonzeros);
    transpose_graph(V, E, fwd_offsets, fwd_nonzeros, rev_offsets, rev_nonzeros);
    // std::cout << "V = " << V << ", E = " << E << std::endl;

    g_fwd_offsets = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * (V + 1));
    g_fwd_edges = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * E);
    g_rev_offsets = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * (V + 1));
    g_rev_edges = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * E);
    g_distance = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * V);
    g_V = V;
    g_E = E;
    g_rev_not_fwd = false;
    g_mf = 0;
    g_mu = 0;

    // initialize frontier data
    for (size_t i = 0; i < g_frontier.size(); i++) {
      frontier_ref frontier = &g_frontier[i];
      frontier.size() = 0;
      frontier.is_dense() = true;
      frontier.vertices() = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * V);
    }
    curr_frontier.size() = 1;
    curr_frontier.is_dense() = false;
    curr_frontier.vertices(0) = vertex;

    // run referecne
    breadth_first_search_graph(vertex, V, E, fwd_offsets, fwd_nonzeros, distance);
  }
  barrier.sync([]() {
    std::cout << "Finished reading graph" << std::endl;
  });

  // copy graph memory model
  int V = g_V;
  int E = g_E;

  for (int v = my_thread(); v < V + 1; v += THREADS) {
    g_fwd_offsets[v] = fwd_offsets[v];
    g_rev_offsets[v] = rev_offsets[v];
  }
  for (int e = my_thread(); e < E; e += THREADS) {
    g_fwd_edges[e] = fwd_nonzeros[e];
    g_rev_edges[e] = rev_nonzeros[e];
  }
  for (int v = my_thread(); v < V; v += THREADS) {
    g_distance[v] = -1;
  }

  barrier.sync([]() {
    g_distance[root_vertex] = 0;
    std::cout << "Starting BFS (Push-Pull)" << std::endl;
  });

  // until the frontier is empty

  Pointer<int> l_distance = g_distance;
  Pointer<int> l_fwd_offsets = g_fwd_offsets;
  Pointer<int> l_fwd_edges = g_fwd_edges;
  Pointer<int> l_rev_offsets = g_rev_offsets;
  Pointer<int> l_rev_edges = g_rev_edges;

  // std::cout << "Thread " << my_thread()
  //           << ": curr_frontier = " << std::hex << static_cast<uint64_t>(&curr_frontier) <<
  //           std::dec << std::endl;
  // std::cout << "Thread " << my_thread()
  //           << ": next_frontier = " << std::hex << static_cast<uint64_t>(&next_frontier) <<
  //           std::dec << std::endl;
  int iter = 0;
  while (curr_frontier.size() != 0) {
    int d = iter + 1;
    barrier.sync([=]() {
      std::cout << "Iteration " << std::setw(2) << iter << ": " << std::setw(3)
                << curr_frontier.size() << " in frontier" << std::endl;
      g_mu = 0;
      g_mf = 0;
    });

    ////////////////////////////////////////////
    // decide the direction of this iteration //
    ////////////////////////////////////////////
    if (!g_rev_not_fwd) {
      // make frontier sparse if needed
      curr_frontier = curr_frontier.to_sparse(tmp_frontier, barrier);

      // do we switch from forward to reverse?
      // 1. find sum (degree in frontier)
      int mf_local = 0;
      for (int src_i = my_thread(); src_i < curr_frontier.size(); src_i += THREADS) {
        // todo: hoist reading the pointer vertices out of this loop
        int src = curr_frontier.vertices(src_i);
        mf_local += l_fwd_offsets[src + 1] - l_fwd_offsets[src];
      }
      DrvAPI::atomic_add(&g_mf, mf_local);
      // 2. find sum (degree unvisited)
      int mu_local = 0;
      for (int v = my_thread(); v < V; v += THREADS) {
        if (l_distance[v] == -1) {
          mu_local += l_fwd_offsets[v + 1] - l_fwd_offsets[v];
        }
      }
      DrvAPI::atomic_add(&g_mu, mu_local);
      barrier.sync([=]() {
        g_rev_not_fwd = (g_mf > (g_mu / 20));
        std::cout << "Iteration " << std::setw(2) << iter << ": " << std::setw(3) << g_mf << " mf, "
                  << std::setw(3) << g_mu << " mu, " << (g_rev_not_fwd ? "rev" : "fwd")
                  << std::endl;
      });
    } else {
      // do we switch from reverse to forward?
      // printf("Thread %d: deciding to switch back\n", my_thread());
      barrier.sync([=]() {
        g_rev_not_fwd = (curr_frontier.size() >= (V / 20));
        std::cout << "Iteration " << std::setw(2) << iter
                  << ": curr_frontier.size()=" << std::setw(3) << curr_frontier.size()
                  << ", (V/20) = (" << V << "/20) = " << (V / 20) << ", "
                  << (g_rev_not_fwd ? "rev" : "fwd") << std::endl;
      });
    }
    barrier.sync();

    if (g_rev_not_fwd) {
      // make frontier dense if needed
      curr_frontier = curr_frontier.to_dense(tmp_frontier, barrier);

      // traverse backards
      int contrib = 0;
      for (int dst = my_thread(); dst < V; dst += THREADS) {
        if (l_distance[dst] == -1) {
          int src_start = l_rev_offsets[dst];
          int src_stop = l_rev_offsets[dst + 1];
          for (int src_i = src_start; src_i < src_stop; src_i++) {
            int src = l_rev_edges[src_i];
            if (curr_frontier.vertices(src) == 1) {
              l_distance[dst] = d;
              next_frontier.vertices(dst) = 1;
              contrib++;
              break;
            }
          }
        }
      }
      DrvAPI::atomic_add(&next_frontier.size(), contrib);

      // wait for all threads to finish
      barrier.sync();

      // swap frontiers
      swap(curr_frontier, next_frontier);
      next_frontier.clear(barrier);
    } else {
      // make frontier sparse if needed
      curr_frontier = curr_frontier.to_sparse(tmp_frontier, barrier);

      // iterate over vertices in the frontier
      int size = curr_frontier.size();
      int contrib = 0;
      for (int src_i = my_thread(); src_i < size; src_i += THREADS) {
        int src = curr_frontier.vertices(src_i);
        int dst_start = l_fwd_offsets[src];
        int dst_stop = l_fwd_offsets[src + 1];
        for (int dst_i = dst_start; dst_i < dst_stop; dst_i++) {
          int dst = l_fwd_edges[dst_i];
          if (l_distance[dst] == -1) {
            if (DrvAPI::atomic_swap(&l_distance[dst], d) == -1) {
              next_frontier.vertices(dst) = 1;
              contrib++;
            }
          }
        }
      }

      DrvAPI::atomic_add(&next_frontier.size(), contrib);

      // wait for all threads to finish
      barrier.sync([=]() {
        // std::cout << "Iteration " << std::setw(2) << iter
        //           << ": " << std::setw(3) << contrib
        //           << " new vertices" << std::endl;
      });

      // swap frontiers
      swap(curr_frontier, next_frontier);
      next_frontier.clear(barrier);
    }
    // increment iteration
    iter++;
  }

  // check the result
  if (my_thread() == 0) {
    for (int v = 0; v < V; v++) {
      if (l_distance[v] != distance[v]) {
        std::cout << "ERROR: distance[" << v << "] = " << l_distance[v] << ", expected "
                  << distance[v] << std::endl;
      }
      // std::cout << "distance[" << v << "] = " << g_distance[v] << std::endl;
    }
  }
  barrier.sync();
  return 0;
}

declare_drv_api_main(BFSMain);
