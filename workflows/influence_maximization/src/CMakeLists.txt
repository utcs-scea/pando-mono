# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-wf4-galois target
#

target_sources(pando-wf4-galois
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/import.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/influence_maximization.cpp
)

pando_compiler_options(pando-wf4-galois)
pando_compiler_warnings(pando-wf4-galois)

# link pando-lib-galois
target_include_directories(pando-wf4-galois
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/pando-rt/include
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/include
)

target_link_libraries(pando-wf4-galois
    PUBLIC
        pando-lib-galois::pando-lib-galois
)

if (PANDO_RT_BACKEND STREQUAL "DRVX")
    add_library(wf4 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
else()
    add_executable(wf4 ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
endif()

target_include_directories(wf4
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../include
)

target_link_libraries(wf4
    PUBLIC
        pando-wf4-galois::pando-wf4-galois
)
