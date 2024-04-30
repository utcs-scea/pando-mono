// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <utils.hpp>

template <typename GraphType>
void show_graph(pando::GlobalPtr<GraphType> graph_ptr) {
  for (typename GraphType::VertexTopologyID vertex : graph_ptr->vertices()) {
    std::cout << "VERTEX: " << graph_ptr->getTokenID(vertex)
              << " (numEdges = " << graph_ptr->getNumEdges(vertex) << ") :\n";
    for (typename GraphType::EdgeHandle eh : graph_ptr->edges(vertex)) {
      typename GraphType::VertexTopologyID dst = graph_ptr->getEdgeDst(eh);
      std::cout << "\t" << graph_ptr->getTokenID(dst) << "\n";
    }
  }
}

void show_mirrored_graph(pando::GlobalPtr<GraphMDL> graph_ptr) {
  uint32_t num_hosts = pando::getPlaceDims().node.id;
  std::cout << graph_ptr->vertices().size() << " = VERTEX SIZE\n";
  for (uint32_t i = 0; i < num_hosts; i++) {
    std::cout << "*** HOST " << i << "***\n";
    GraphMDL::LocalVertexRange masterRange = graph_ptr->getMasterRange(i);
    GraphMDL::LocalVertexRange mirrorRange = graph_ptr->getMirrorRange(i);
    std::cout << "*** num_masters = " << masterRange.size()
              << " ... num_mirrors = " << mirrorRange.size() << "\n";
    for (GraphMDL::VertexTopologyID masterTopologyID = *lift(masterRange, begin);
         masterTopologyID < *lift(masterRange, end); masterTopologyID++) {
      auto master_tokenID = graph_ptr->getTokenID(masterTopologyID);
      auto global_topo_id = graph_ptr->getGlobalTopologyID(master_tokenID);
      std::cout << "\tMASTER: " << graph_ptr->getTokenID(global_topo_id) << "\n";
      for (typename GraphMDL::EdgeHandle eh : graph_ptr->edges(global_topo_id)) {
        typename GraphMDL::VertexTopologyID dst = graph_ptr->getEdgeDst(eh);
        std::cout << "\t\tDST:" << graph_ptr->getTokenID(dst) << "\n";
      }
    }
  }
}

pando::Status generate_mirrored_graph_stats(pando::GlobalPtr<GraphMDL> graph_ptr) {
  std::ofstream stat_file;
  std::string path = "data/graph_stats_MirroredDistLocalCSR.csv";

  stat_file.open(path, std::ios::trunc);
  if (!stat_file.is_open()) {
    std::cerr << "Failed to open file:" << path << "\n";
    return pando::Status::Error;
  }

  uint64_t num_hosts = pando::getPlaceDims().node.id;
  std::cerr << "Successfully open file::: num_hosts = " << num_hosts << " \n";
  galois::HostIndexedMap<uint64_t> num_masters_per_host{};
  galois::HostIndexedMap<uint64_t> num_mirrors_per_host{};
  galois::HostIndexedMap<uint64_t> num_vertices_per_host{};
  galois::HostIndexedMap<uint64_t> num_edges_per_host{};
  PANDO_CHECK_RETURN(num_masters_per_host.initialize());
  PANDO_CHECK_RETURN(num_mirrors_per_host.initialize());
  PANDO_CHECK_RETURN(num_vertices_per_host.initialize());
  PANDO_CHECK_RETURN(num_edges_per_host.initialize());

  // Initialize counts to 0
  for (uint64_t i = 0; i < num_hosts; i++) {
    num_masters_per_host[i] = 0;
    num_mirrors_per_host[i] = 0;
    num_vertices_per_host[i] = 0;
    num_edges_per_host[i] = 0;
  }

  // Collect Data
  std::cout << "Collecting vertex-edge data\n";
  for (typename GraphMDL::VertexTopologyID vertex : graph_ptr->vertices()) {
    uint64_t host_id = static_cast<std::uint64_t>(graph_ptr->getLocalityVertex(vertex).node.id);
    num_vertices_per_host[host_id]++;
    for (typename GraphMDL::EdgeHandle eh : graph_ptr->edges(vertex)) {
      pando::GlobalPtr<typename GraphMDL::EdgeData> ptr = &graph_ptr->getEdgeData(eh);
      uint64_t eh_host_id = static_cast<std::uint64_t>(galois::localityOf(ptr).node.id);
      num_edges_per_host[eh_host_id]++;
    }
  }

  std::cout << "Collecting master data \n";
  galois::doAll(
      graph_ptr, num_masters_per_host,
      +[](pando::GlobalPtr<GraphMDL> graph_ptr, pando::GlobalRef<std::uint64_t> master_ref) {
        GraphMDL graph = *graph_ptr;
        master_ref = graph.getMasterSize();
      });

  std::cout << "Collecting mirror data \n";
  galois::doAll(
      graph_ptr, num_mirrors_per_host,
      +[](pando::GlobalPtr<GraphMDL> graph_ptr, pando::GlobalRef<std::uint64_t> mirror_ref) {
        GraphMDL graph = *graph_ptr;
        mirror_ref = graph.getMirrorSize();
      });

  stat_file << "Host,Category,Count\n";
  for (uint64_t i = 0; i < num_hosts; i++) {
    uint64_t host_i_num_master = num_masters_per_host[i];
    uint64_t host_i_num_mirror = num_mirrors_per_host[i];
    uint64_t host_i_num_vert = num_vertices_per_host[i];
    uint64_t host_i_num_edges = num_edges_per_host[i];
    stat_file << i << ",Masters," << host_i_num_master << "\n";
    stat_file << i << ",Mirrors," << host_i_num_mirror << "\n";
    stat_file << i << ",Vertices," << host_i_num_vert << "\n";
    stat_file << i << ",Edges," << host_i_num_edges << "\n";
    std::cerr << "Host " << i << ": master, mirror, V, E = " << host_i_num_master << ", "
              << host_i_num_mirror << ", " << host_i_num_vert << ", " << host_i_num_edges << "\n";
  }

  std::cerr << "De-initing and closing file\n";
  num_masters_per_host.deinitialize();
  num_mirrors_per_host.deinitialize();
  num_vertices_per_host.deinitialize();
  num_edges_per_host.deinitialize();

  stat_file.close();
  return pando::Status::Success;
}

template <typename GraphType>
pando::Status generate_graph_stats(pando::GlobalPtr<GraphType> graph_ptr,
                                   GRAPH_TYPE graph_type_enum) {
  std::ofstream stat_file;
  std::string graph_type = (graph_type_enum == GRAPH_TYPE::DLCSR)    ? "DistLocalCSR"
                           : (graph_type_enum == GRAPH_TYPE::DACSR)  ? "DistArrayCSR"
                           : (graph_type_enum == GRAPH_TYPE::MDLCSR) ? "MirroredDistLocalCSR"
                                                                     : "Invalid";
  std::string path = "data/graph_stats_" + graph_type + ".csv";

  stat_file.open(path, std::ios::trunc);
  if (!stat_file.is_open()) {
    std::cerr << "Failed to open file:" << path << "\n";
    return pando::Status::Error;
  }

  uint64_t num_hosts = pando::getPlaceDims().node.id;
  std::cerr << "Successfully open file::: num_hosts = " << num_hosts << " \n";
  galois::HostIndexedMap<uint64_t> num_vertices_per_host{};
  galois::HostIndexedMap<uint64_t> num_edges_per_host{};
  PANDO_CHECK_RETURN(num_vertices_per_host.initialize());
  PANDO_CHECK_RETURN(num_edges_per_host.initialize());

  // Initialize counts to 0
  for (uint64_t i = 0; i < num_hosts; i++) {
    num_vertices_per_host[i] = 0;
    num_edges_per_host[i] = 0;
  }

  // Collect Data
  for (typename GraphType::VertexTopologyID vertex : graph_ptr->vertices()) {
    uint64_t host_id = static_cast<std::uint64_t>(graph_ptr->getLocalityVertex(vertex).node.id);
    num_vertices_per_host[host_id]++;
    for (typename GraphType::EdgeHandle eh : graph_ptr->edges(vertex)) {
      pando::GlobalPtr<typename GraphType::EdgeData> ptr = &graph_ptr->getEdgeData(eh);
      uint64_t eh_host_id = static_cast<std::uint64_t>(galois::localityOf(ptr).node.id);
      num_edges_per_host[eh_host_id]++;
    }
  }

  stat_file << "Host,Category,Count\n";
  for (uint64_t i = 0; i < num_hosts; i++) {
    uint64_t host_i_num_vert = num_vertices_per_host[i];
    uint64_t host_i_num_edges = num_edges_per_host[i];
    stat_file << i << ",Vertices," << host_i_num_vert << "\n";
    stat_file << i << ",Edges," << host_i_num_edges << "\n";
    std::cerr << "Host " << i << ": V, E = " << host_i_num_vert << ", " << host_i_num_edges << "\n";
  }

  std::cerr << "De-initing and closing file\n";
  num_vertices_per_host.deinitialize();
  num_edges_per_host.deinitialize();

  stat_file.close();
  return pando::Status::Success;
}

void HBMainGraphCompare(pando::Notification::HandleType hb_done, pando::Array<char> filename,
                        int64_t num_vertices, GRAPH_TYPE graph_type) {
  switch (graph_type) {
    case GRAPH_TYPE::MDLCSR: {
      std::cout << "Creating MDLCSR ...\n";
      GraphMDL graph = galois::initializeELDLCSR<GraphMDL, MirroredVT, ET>(filename, num_vertices);
      pando::GlobalPtr<GraphMDL> graph_ptr = static_cast<pando::GlobalPtr<GraphMDL>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphMDL)));
      *graph_ptr = graph;
      std::cout << "Collecting Graph Stats ...\n";
      PANDO_CHECK(generate_mirrored_graph_stats(graph_ptr));
      pando::deallocateMemory(graph_ptr, 1);
      graph.deinitialize();
      break;
    }
    case GRAPH_TYPE::DACSR: {
      std::cout << "Creating DACSR ...\n";
      GraphDA graph = galois::initializeELDACSR<GraphDA, VT, ET>(filename, num_vertices);
      pando::GlobalPtr<GraphDA> graph_ptr = static_cast<pando::GlobalPtr<GraphDA>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDA)));
      *graph_ptr = graph;
      std::cout << "Collecting Graph Stats ...\n";
      PANDO_CHECK(generate_graph_stats(graph_ptr, graph_type));
      pando::deallocateMemory(graph_ptr, 1);
      graph.deinitialize();
      break;
    }
    default: {
      std::cout << "Creating DLCSR ...\n";
      GraphDL graph = galois::initializeELDLCSR<GraphDL, VT, ET>(filename, num_vertices);
      pando::GlobalPtr<GraphDL> graph_ptr = static_cast<pando::GlobalPtr<GraphDL>>(
          pando::getDefaultMainMemoryResource()->allocate(sizeof(GraphDL)));
      *graph_ptr = graph;
      std::cout << "Collecting Graph Stats ...\n";
      PANDO_CHECK(generate_graph_stats(graph_ptr, graph_type));
      pando::deallocateMemory(graph_ptr, 1);
      graph.deinitialize();
      break;
    }
  }
  std::cout << "DONE\n";
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
                                 opts->num_vertices, opts->graph_type));
    necessary.wait();

    filename.deinitialize();
  }
  pando::waitAll();
  return 0;
}
