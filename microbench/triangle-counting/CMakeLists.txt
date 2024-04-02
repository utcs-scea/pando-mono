# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${PROJECT_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-tc-galois target
#

#if (NOT PANDO_RT_BACKEND STREQUAL "DRVX")
target_sources(pando-tc-galois
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tc_algos.cpp
)
#endif()

pando_compiler_options(pando-tc-galois)
pando_compiler_warnings(pando-tc-galois)

# link pando-lib-galois
target_include_directories(pando-tc-galois
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/pando-rt/include
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/include
)

target_link_libraries(pando-tc-galois
    PUBLIC pando-lib-galois::pando-lib-galois)

function(tc_add_executable TARGET)
    if (PANDO_RT_BACKEND STREQUAL "DRVX")
        add_library(${TARGET} SHARED ${ARGN})
    else()
        add_executable(${TARGET} ${ARGN})
    endif()

    # link pando-lib-galois
    target_link_libraries(${TARGET} PUBLIC pando-tc-galois::pando-tc-galois)

    target_include_directories(${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/../include
    )
    pando_compiler_warnings(${TARGET})
    pando_compiler_options(${TARGET})
endfunction()


tc_add_executable(tc ${CMAKE_CURRENT_SOURCE_DIR}/tc.cpp)
tc_add_executable(graph-compare ${CMAKE_CURRENT_SOURCE_DIR}/graph_compare.cpp)
