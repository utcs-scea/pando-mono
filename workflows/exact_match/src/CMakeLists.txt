# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-wf2-galois target
#

target_sources(pando-wf2-galois
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/exact_pattern.cpp
)

target_include_directories(pando-wf2-galois
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/pando-rt/include
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/include
)

pando_add_executable(wf2 ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_include_directories(wf2
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../include
)

if (PANDO_RT_BACKEND STREQUAL "DRVX")
    target_link_libraries(wf2
        PUBLIC
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-wf2-galois::pando-wf2-galois>"
    )
else()
    target_link_libraries(wf2
        PUBLIC
            pando-wf2-galois::pando-wf2-galois
    )
endif()
