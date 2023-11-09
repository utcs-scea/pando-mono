/* Copyright (c) 2023. University of Texas at Austin. All rights reserved. */
#ifndef PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <fstream>
#include <utility>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

namespace galois {

/**
 * @brief Imports an edge list to a vector based CSR
 *
 * @param[in] numNodes the number of nodes that are under examination
 * @param[in] filePath the path to the edge list file
 * @param[in] retRef the reference to the Vector to put values into.
 */
pando::Status importNaiveCSRFromEdgeListFileOnCPU(
    std::uint64_t numNodes, char* filePath,
    pando::GlobalRef<pando::Vector<pando::Vector<std::uint64_t>>> retRef);

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_
