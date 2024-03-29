// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <DrvAPI.hpp>
#include <pandocommand/loader.hpp>
#include <pandocommand/control.hpp>
#include <pandocommand/executable.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <inttypes.h>
#include "common.hpp"
#include "breadth_first_search_graph.hpp"
#include "read_graph.hpp"
#include "transpose_graph.hpp"

using namespace DrvAPI;
using pandocommand::Place;
enum {
    ARG_THIS_EXE = 0,
    ARG_PH_EXE = 1,
    ARG_GRAPH_FILE = 2,
    ARG_ROOT_VERTEX = 3,
};

class PANDOHammerExe : public pandocommand::PANDOHammerExe {
public:
    PANDOHammerExe() : pandocommand::PANDOHammerExe() {
        bump_allocator_init();
    }
    PANDOHammerExe(const char *fname): pandocommand::PANDOHammerExe(fname) {
        bump_allocator_init();
    }

    static std::shared_ptr<PANDOHammerExe> Open(const char *fname) {
        return std::make_shared<PANDOHammerExe>(fname);
    }

    template <typename T>
    DrvAPIPointer<T> allocate(size_t size) {
        // minimum allocation size is 16B
        size = std::max(size, (size_t)16);
        // align to 16B
        size = (size + 15) & ~15;
        // allocate
        auto ret = bump_allocator_;
        bump_allocator_ += size;
        return ret;
    }

    void bump_allocator_init() {
        // add 1MB to the end of data section
        bump_allocator_ = symbol<char>("end", Place{0,0,0,0}) + 1024*1024;
        // align to 4KB
        bump_allocator_ = (bump_allocator_ + 4095) & ~4095;
    }

    DrvAPI::DrvAPIPointer<char> bump_allocator_;
};

std::vector<int> fwd_offsets; //!< offsets for the forward graph
std::vector<int> fwd_nonzeros; //!< nonzeros for the forward graph
std::vector<int> rev_offsets; //!< offsets for the reverse graph
std::vector<int> rev_nonzeros; //!< nonzeros for the reverse graph
std::vector<int> distance; //!< distance from the root vertex
int V, E; //!< number of vertices and edges
int root_vertex; //!< root vertex

int CommandProcessor(int argc, char *argv[])
{
    // get inputs
    std::string graph_file = "";
    std::string root_vertex_str = "0";
    if (argc > ARG_GRAPH_FILE) {
        graph_file = argv[ARG_GRAPH_FILE];
    }
    if (argc > ARG_ROOT_VERTEX) {
        root_vertex_str = argv[ARG_ROOT_VERTEX];
    }

    printf("Opening graph file: %s\n", graph_file.c_str());
    read_graph(graph_file, &V, &E, fwd_offsets, fwd_nonzeros);
    transpose_graph(V, E, fwd_offsets, fwd_nonzeros, rev_offsets, rev_nonzeros);

    printf("Vertices: %d, Edges: %d\n", V, E);

    printf("Root vertex: %s\n", root_vertex_str.c_str());
    root_vertex = std::stoi(root_vertex_str);

    // run reference
    breadth_first_search_graph(root_vertex, V, E, fwd_offsets, fwd_nonzeros, distance);

    // open the PH executable
    auto ph_exe = PANDOHammerExe::Open(argv[ARG_PH_EXE]);
    Place place{0,0,0,0};
    pandocommand::loadProgram(*ph_exe);
    pandocommand::assertResetAll(false);

    // synchronize witht the ph cores
    auto cp_ready = ph_exe->symbol<int64_t>("cp_ready", place);
    auto ph_ready = ph_exe->symbol<int64_t>("ph_ready", place);
    auto ph_done  = ph_exe->symbol<int64_t>("ph_done", place);

    // wait for PH to be ready
    // it's important to wait for the PH first
    // since it needs to complete loading
    int64_t num_ready = 0;
    printf("CP: waiting for PH threads to be ready: Cores: %d, Threads/Core: %d\n"
           ,numPodCores(), THREADS_PER_CORE);

    while ((num_ready = *ph_ready) < THREADS_PER_CORE*numPodCores()) {
        printf("CP: num PH threads ready = %" PRId64 "\n", num_ready);
        DrvAPI::wait(100);
    }
    printf("CP: all PH threads ready\n");

    // move graph data to the PH
    auto g_fwd_offsets_p = ph_exe->symbol<vertex_pointer_t>("g_fwd_offsets", place);
    auto g_fwd_edges_p = ph_exe->symbol<vertex_pointer_t>("g_fwd_edges", place);
    auto g_rev_offsets_p = ph_exe->symbol<vertex_pointer_t>("g_rev_offsets", place);
    auto g_rev_edges_p = ph_exe->symbol<vertex_pointer_t>("g_rev_edges", place);
    auto g_distance_p = ph_exe->symbol<vertex_pointer_t>("g_distance", place);
    auto g_V_p = ph_exe->symbol<vertex_t>("g_V", place);
    auto g_E_p = ph_exe->symbol<vertex_t>("g_E", place);
    auto g_rev_not_fwd_p = ph_exe->symbol<bool>("g_rev_not_fwd", place);
    auto g_mf_p = ph_exe->symbol<int>("g_mf", place);
    auto g_mu_p = ph_exe->symbol<int>("g_mu", place);

    *g_V_p = V;
    *g_E_p = E;
    auto g_fwd_offsets = ph_exe->allocate<vertex_t>(sizeof(vertex_t)*(V+1));
    *g_fwd_offsets_p = g_fwd_offsets;
    auto g_fwd_edges = ph_exe->allocate<vertex_t>(sizeof(vertex_t)*E);
    *g_fwd_edges_p = g_fwd_edges;
    auto g_rev_offsets = ph_exe->allocate<vertex_t>(sizeof(vertex_t)*(V+1));
    *g_rev_offsets_p = g_rev_offsets;
    auto g_rev_edges = ph_exe->allocate<vertex_t>(sizeof(vertex_t)*E);
    *g_rev_edges_p = g_rev_edges;
    auto g_distance = ph_exe->allocate<vertex_t>(sizeof(vertex_t)*V);
    *g_distance_p = g_distance;
    *g_rev_not_fwd_p = false;
    *g_mf_p = 0;
    *g_mu_p = 0;

    auto frontiers = ph_exe->symbol<frontier_data>("frontier", place);

    frontier_ref curr = &frontiers[0];
    frontier_ref next = &frontiers[1];
    frontier_ref resv = &frontiers[2];

    // initialize frontier data
    for (int i = 0; i < 3; i++) {
        frontier_ref f = &frontiers[i];
        f.size() = 0;
        f.vertices() = ph_exe->allocate<vertex_t>(sizeof(vertex_t) * V);
        f.is_dense() = true;
    }
    curr.size() = 1;
    curr.is_dense() = false;
    curr.vertices(0) = root_vertex;

    // copy graph memory model
    for (vertex_t v = 0; v < V+1; v += 1) {
        g_fwd_offsets[v] = fwd_offsets[v];
        g_rev_offsets[v] = rev_offsets[v];
    }
    for (vertex_t e = 0; e < E; e += 1) {
        g_fwd_edges[e] = fwd_nonzeros[e];
        g_rev_edges[e] = rev_nonzeros[e];
    }
    for (vertex_t v = 0; v < V; v += 1) {
        g_distance[v] = -1;
    }
    g_distance[root_vertex] = 0;

    // FENCE here

    *cp_ready = 1; // signal to ph core's that we are ready
    double start_s = DrvAPI::seconds();
    printf("starting BFS @ %lf s\n", start_s);
    // wait for ph done
    int64_t num_done = 0;
    while ((num_done = *ph_done) < THREADS_PER_CORE*numPodCores()) {
        DrvAPI::wait(1000);
    }

    printf("CP: all PH threads are done (%" PRId64 ")\n", num_done);
    double end_s = DrvAPI::seconds();
    printf("stoping  BFS @ %lf s\n", end_s);
    printf("elapsed  BFS time: %lf s\n", end_s - start_s);

    for (vertex_t v = 0; v < V; v++) {
        if (g_distance[v] != distance[v]) {
            printf("ERROR: distance[%d] = %d, expected %d\n"
                   ,v
                   ,(vertex_t)g_distance[v]
                   ,distance[v]);
        }
    }

    return 0;
}

declare_drv_api_main(CommandProcessor);
