// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#ifndef PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_
#define PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_

#include <pando-rt/export.h>

#include <fstream>
#include <utility>

#include <pando-rt/containers/vector.hpp>
#include <pando-rt/pando-rt.hpp>

/** @file edge_list_importer.hpp
 *  This edge list importer reads an edge list file, and constructs a 2D array that
 *  consists of an edge list for each vertex.
 *  The output list of the edge lists will be the input for CSR graph construction.
 */

namespace galois {

/**
 * @brief Imports an edge list to a vector based CSR
 *
 * @param[in] numVertices the number of Vertices; the reason why this is an input is because
 *            we want to materialize a dense vector for edge end points.
 * @param[in] filePath the path to the edge list file
 * @param[out] elRef the reference to the output edge list
 */
pando::Status importELFile(std::uint64_t numVertices, const char* filePath,
                           pando::GlobalRef<pando::Vector<pando::Vector<std::uint64_t>>> elRef);

} // namespace galois

#endif // PANDO_LIB_GALOIS_GRAPHS_EDGE_LIST_IMPORTER_HPP_
