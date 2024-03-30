# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-lib-galois target
#

target_include_directories(pando-lib-galois
    PUBLIC
        "$<BUILD_INTERFACE:${pando-lib-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_BINARY_DIR}/include/pando-lib-galois>"
)

target_sources(pando-lib-galois
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/atomic.cpp
        ${CMAKE_CURRENT_LIST_DIR}/loops.cpp
        ${CMAKE_CURRENT_LIST_DIR}/edge_list_importer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ifstream.cpp
        ${CMAKE_CURRENT_LIST_DIR}/schema.cpp
        ${CMAKE_CURRENT_LIST_DIR}/edge_exchange.cpp
        ${CMAKE_CURRENT_LIST_DIR}/wmd_graph_importer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ingest_wmd_csv.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ingest_rmat_el.cpp
        ${CMAKE_CURRENT_LIST_DIR}/local_storage.cpp
)

pando_compiler_options(pando-lib-galois)
pando_compiler_warnings(pando-lib-galois)

# link pando-rt
target_include_directories(pando-lib-galois
    PUBLIC
        ${pando-lib-galois_BINARY_DIR}/pando-rt/include
)

if (PANDO_RT_BACKEND STREQUAL "DRVX")
    target_link_libraries(pando-lib-galois PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-rt::pando-rt>")
else()
    target_link_libraries(pando-lib-galois
    PUBLIC
        pando-rt::pando-rt
)
endif()
