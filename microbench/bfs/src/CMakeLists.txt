# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-bfs-galois target
#

target_sources(pando-bfs
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

# link pando-lib-galois
target_include_directories(pando-bfs
    PUBLIC
        ${pando-lib-galois_BINARY_DIR}/pando-rt/include
        ${pando-lib-galois_BINARY_DIR}/include
)

pando_add_executable(bfs ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_include_directories(bfs
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../include
)

if (PANDO_RT_BACKEND STREQUAL "DRVX")
    target_link_libraries(bfs
        PUBLIC
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-bfs::pando-bfs>"
    )
else()
    target_link_libraries(bfs
        PUBLIC
            pando-bfs::pando-bfs
    )
endif()
