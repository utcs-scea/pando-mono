// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <utils.hpp>

template <typename GraphType>
void show_graph(pando::GlobalPtr<GraphType> graph_ptr) {
  for (typename GraphType::VertexTopologyID vertex : graph_ptr->vertices()) {
    std::cout << "VERTEX: " << graph_ptr->getTokenID(vertex)
              << " (numEdges = " << graph_ptr->getNumEdges(vertex) << ")"
              << ":\n";
    for (typename GraphType::EdgeHandle eh : graph_ptr->edges(vertex)) {
      typename GraphType::VertexTopologyID dst = graph_ptr->getEdgeDst(eh);
      std::cout << "\t" << graph_ptr->getTokenID(dst) << "\n";
    }
#if SORTED_EDGES
    auto src_edges = graph_ptr->edges(vertex);
    auto end_ptr = src_edges.end();
    end_ptr--;
    for (auto it = src_edges.begin(); it != end_ptr && it != src_edges.end(); it++) {
      typename GraphType::VertexTopologyID dst0 = graph_ptr->getEdgeDst(*it);
      typename GraphType::VertexTopologyID dst1 = graph_ptr->getEdgeDst(*(it + 1));
      if (graph_ptr->getTokenID(dst0) >= graph_ptr->getTokenID(dst1)) {
        std::cout << "BAD GRAPH ORDERING (NOT SORTED), as Src " << graph_ptr->getTokenID(vertex)
                  << " has edges in wrong order: " << graph_ptr->getTokenID(dst0) << ", "
                  << graph_ptr->getTokenID(dst1) << "\n";
      }
    }
#endif
  }
}
template <typename GraphType>
pando::Status generate_graph_stats(pando::GlobalPtr<GraphType> graph_ptr,
                                   bool load_balanced_graph) {
  std::ofstream stat_file;
  std::string graph_type = (load_balanced_graph ? "DistLocalCSR" : "DistArrayCSR");
  std::string path = "data/graph_stats_" + graph_type + ".csv";
#if DEBUG
  std::ofstream graph_file;
  std::string graph_dump_path = "data/graph_dmp.el";
#endif

  stat_file.open(path, std::ios::trunc);
  if (!stat_file.is_open()) {
    std::cerr << "Failed to open file:" << path << "\n";
    return pando::Status::Error;
  }

#if DEBUG
  graph_file.open(graph_dump_path, std::ios::trunc);
  if (!graph_file.is_open()) {
    std::cerr << "Failed to open file:" << graph_dump_path << "\n";
    return pando::Status::Error;
  }
#endif

  uint64_t num_hosts = pando::getPlaceDims().node.id;
  std::cerr << "Successfully open file::: num_hosts = " << num_hosts << " \n";
  galois::HostIndexedMap<uint64_t> num_vertices_per_host{};
  galois::HostIndexedMap<uint64_t> num_edges_per_host{};
  PANDO_CHECK_RETURN(num_vertices_per_host.initialize());
  PANDO_CHECK_RETURN(num_edges_per_host.initialize());

  // Initialize counts to 0
  for (uint64_t i = 0; i < num_hosts; i++) {
    num_vertices_per_host.get(i) = 0;
    num_edges_per_host.get(i) = 0;
  }

  // Collect Data
  for (typename GraphType::VertexTopologyID vertex : graph_ptr->vertices()) {
    uint64_t host_id = static_cast<std::uint64_t>(graph_ptr->getLocalityVertex(vertex).node.id);
    num_vertices_per_host.get(host_id)++;
    for (typename GraphType::EdgeHandle eh : graph_ptr->edges(vertex)) {
      pando::GlobalPtr<typename GraphType::EdgeData> ptr = &graph_ptr->getEdgeData(eh);
      uint64_t eh_host_id = static_cast<std::uint64_t>(galois::localityOf(ptr).node.id);
      num_edges_per_host.get(eh_host_id)++;
    }
  }

  stat_file << "Host,Category,Count\n";
  for (uint64_t i = 0; i < num_hosts; i++) {
    stat_file << i << ",Vertices," << num_vertices_per_host.get(i) << "\n";
    stat_file << i << ",Edges," << num_edges_per_host.get(i) << "\n";
    std::cerr << "Host " << i << ": V, E = " << num_vertices_per_host.get(i) << ", "
              << num_edges_per_host.get(i) << "\n";
  }

  std::cerr << "De-initing and closing file\n";
  num_vertices_per_host.deinitialize();
  num_edges_per_host.deinitialize();

#if DEBUG
  // show_graph(graph_ptr);
  for (typename GraphType::VertexTopologyID vertex : graph_ptr->vertices()) {
    for (typename GraphType::EdgeHandle eh : graph_ptr->edges(vertex)) {
      typename GraphType::VertexTopologyID dst = graph_ptr->getEdgeDst(eh);
      graph_file << graph_ptr->getTokenID(vertex) << " " << graph_ptr->getTokenID(dst) << "\n";
    }
#if SORTED_EDGES
    auto src_edges = graph_ptr->edges(vertex);
    auto end_ptr = src_edges.end();
    end_ptr--;
    for (auto it = src_edges.begin(); it != end_ptr && it != src_edges.end(); it++) {
      typename GraphType::VertexTopologyID dst0 = graph_ptr->getEdgeDst(*it);
      typename GraphType::VertexTopologyID dst1 = graph_ptr->getEdgeDst(*(it + 1));
      if (graph_ptr->getTokenID(dst0) >= graph_ptr->getTokenID(dst1)) {
        std::cout << "BAD GRAPH ORDERING (NOT SORTED), as Src " << graph_ptr->getTokenID(vertex)
                  << " has edges in wrong order: " << graph_ptr->getTokenID(dst0) << ", "
                  << graph_ptr->getTokenID(dst1) << "\n";
        // return pando::Status::Error;
      }
    }
#endif
  }

  graph_file.close();
#endif

  stat_file.close();
  return pando::Status::Success;
}

void HBMainGraphCompare(pando::Notification::HandleType hb_done, pando::Array<char> filename,
                        int64_t num_vertices, bool load_balanced_graph) {
  if (load_balanced_graph) {
    GraphDL graph =
        galois::initializeELDLCSR<galois::ELVertex, galois::ELEdge>(filename, num_vertices);
    pando::GlobalPtr<GraphDL> graph_ptr = static_cast<pando::GlobalPtr<GraphDL>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDL)));
    *graph_ptr = graph;
    std::cout << "Collecting Graph Stats ...\n";
    PANDO_CHECK(generate_graph_stats(graph_ptr, load_balanced_graph));
    pando::deallocateMemory(graph_ptr, 1);
    graph.deinitialize();

  } else {
    GraphDA graph =
        galois::initializeELDACSR<galois::ELVertex, galois::ELEdge>(filename, num_vertices);
    pando::GlobalPtr<GraphDA> graph_ptr = static_cast<pando::GlobalPtr<GraphDA>>(
        pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDA)));
    *graph_ptr = graph;
    std::cout << "Collecting Graph Stats ...\n";
    PANDO_CHECK(generate_graph_stats(graph_ptr, load_balanced_graph)); //
    pando::deallocateMemory(graph_ptr, 1);
    graph.deinitialize();
  }
  hb_done.notify();
}

int pandoMain(int argc, char** argv) {
  auto thisPlace = pando::getCurrentPlace();
  std::shared_ptr<CommandLineOptions> opts = read_cmd_line_args(argc, argv);

  if (thisPlace.node.id == COORDINATOR_ID) {
    galois::HostLocalStorageHeap::HeapInit();
    galois::PodLocalStorageHeap::HeapInit();

    pando::Array<char> filename;
    PANDO_CHECK(filename.initialize(opts->elFile.size()));
    for (uint64_t i = 0; i < opts->elFile.size(); i++)
      filename[i] = opts->elFile[i];

    pando::Notification necessary;
    PANDO_CHECK(necessary.init());
    PANDO_CHECK(pando::executeOn(pando::Place{pando::NodeIndex{0}, pando::anyPod, pando::anyCore},
                                 &HBMainGraphCompare, necessary.getHandle(), filename,
                                 opts->num_vertices, opts->load_balanced_graph));
    necessary.wait();

    filename.deinitialize();
  }
  pando::waitAll();
  return 0;
}
