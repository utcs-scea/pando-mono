# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-lib-galois target
#

target_include_directories(pando-lib-galois
    PUBLIC
        "$<BUILD_INTERFACE:${pando-lib-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_BINARY_DIR}/include/pando-lib-galois>"
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

# target
add_library(pando-tc-galois STATIC)
add_library(pando-tc-galois::pando-tc-galois ALIAS pando-tc-galois)


add_subdirectory(triangle-counting)
