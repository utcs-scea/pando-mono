// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "breadth_first_search_graph.hpp"
#include "read_graph.hpp"
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
Global<Pointer<int>> g_offsets;
Global<Pointer<int>> g_edges;
Global<Pointer<int>> g_distance;
// frontier data
struct frontier_data {
  int size;
  Pointer<int> vertices;
};

DRV_API_REF_CLASS_BEGIN(frontier_data)
DRV_API_REF_CLASS_DATA_MEMBER(frontier_data, size)
DRV_API_REF_CLASS_DATA_MEMBER(frontier_data, vertices)
Pointer<int>::value_handle vertices(int i) {
  Pointer<int> v = vertices();
  return v[i];
}
DRV_API_REF_CLASS_END(frontier_data)
using frontier_ref = frontier_data_ref;

Global<frontier_data> g_frontier[2];

std::vector<int> offsets;
std::vector<int> nonzeros;
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
      std::cout << "Usage: " << std::string(argv[0]) << " <graph_file> [root_vertex (default 0)]"
                << std::endl;
      barrier.sync();
      return 1;
    }

    int vertex = std::stoi(root_vertex_str);
    root_vertex = vertex;

    std::cout << "graph_file = " << graph_file << ", root_vertex = " << vertex << std::endl;
    int V, E;
    read_graph(graph_file, &V, &E, offsets, nonzeros);
    std::cout << "V = " << V << ", E = " << E << std::endl;

    g_offsets = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * (V + 1));
    g_edges = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * E);
    g_distance = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * V);
    g_V = V;
    g_E = E;

    // initialize frontier data
    for (int i = 0; i < 2; i++) {
      frontier_ref frontier = &g_frontier[i];
      frontier.size() = 0;
      frontier.vertices() = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, sizeof(int) * V);
    }
    curr_frontier.size() = 1;
    curr_frontier.vertices(0) = vertex;

    // run referecne
    breadth_first_search_graph(vertex, V, E, offsets, nonzeros, distance);
  }
  barrier.sync([]() {
    std::cout << "Finished reading graph" << std::endl;
  });

  // copy graph memory model
  int V = g_V;
  int E = g_E;

  for (int v = my_thread(); v < V + 1; v += THREADS) {
    g_offsets[v] = offsets[v];
  }
  for (int e = my_thread(); e < E; e += THREADS) {
    g_edges[e] = nonzeros[e];
  }
  for (int v = my_thread(); v < V; v += THREADS) {
    g_distance[v] = -1;
  }

  barrier.sync([]() {
    g_distance[root_vertex] = 0;
    std::cout << "Starting BFS" << std::endl;
  });

  // until the frontier is empty
  int d = 0;
  Pointer<int> l_distance = g_distance;
  Pointer<int> l_offsets = g_offsets;
  Pointer<int> l_edges = g_edges;

  while (curr_frontier.size() != 0) {
    barrier.sync([=]() {
      std::cout << "Iteration " << std::setw(2) << d << ": " << std::setw(3) << curr_frontier.size()
                << " in frontier" << std::endl;
    });
    d++;
    // iterate over vertices in the frontier
    int size = curr_frontier.size();
    for (int src_i = my_thread(); src_i < size; src_i += THREADS) {
      int src = curr_frontier.vertices(src_i);
      // mutex.sync([=](){
      //     std::cout << "thread " << myThreadId << ": traversing from src = " << src << std::endl;
      // });
      int dst_start = l_offsets[src];
      int dst_stop = l_offsets[src + 1];
      for (int dst_i = dst_start; dst_i < dst_stop; dst_i++) {
        int dst = l_edges[dst_i];
        if (l_distance[dst] == -1) {
          l_distance[dst] = d;
          next_frontier.vertices(dst) = 1;
        }
      }
    }
    // wait for all threads to finish
    // last thread to finish should clear the current frontier
    barrier.sync([curr_frontier]() {
      curr_frontier.size() = 0;
    });

    // make the next frontier
    for (int v = my_thread(); v < V; v += THREADS) {
      int v_in = DrvAPI::atomic_swap<int>(&next_frontier.vertices(v), 0);
      if (v_in) {
        int idx = DrvAPI::atomic_add<int>(&curr_frontier.size(), 1);
        curr_frontier.vertices(idx) = v;
      }
    }
    barrier.sync();
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
