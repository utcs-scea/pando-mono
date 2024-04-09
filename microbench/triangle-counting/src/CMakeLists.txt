# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#
# pando-triangle-counting target
#

target_include_directories(pando-triangle-counting
    PUBLIC
        "$<BUILD_INTERFACE:${pando-tc-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_BINARY_DIR}/include/pando-lib-galois>"
        ${pando-rt_BINARY_DIR}/include
)

target_sources(pando-triangle-counting
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
        #${CMAKE_CURRENT_LIST_DIR}/tc_algos.cpp
)

target_link_libraries(pando-triangle-counting
    PUBLIC pando-lib-galois::pando-lib-galois)

#if (PANDO_RT_BACKEND STREQUAL "DRVX")
#    target_link_libraries(pando-triangle-counting PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-rt::pando-rt>")
#else()
#    target_link_libraries(pando-triangle-counting
#    PUBLIC
#        pando-rt::pando-rt
#)
#endif()

# target
function(tc_add_executable TARGET)
    if (PANDO_RT_BACKEND STREQUAL "DRVX")
        add_library(${TARGET} SHARED ${ARGN})
    else()
        add_executable(${TARGET} ${ARGN})
    endif()

    # link pando-triangle-counting
    target_link_libraries(${TARGET} PUBLIC pando-triangle-counting::pando-triangle-counting)

    target_include_directories(${TARGET}
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR}/../include
    )
    pando_compiler_warnings(${TARGET})
    pando_compiler_options(${TARGET})
endfunction()

tc_add_executable(bench_remote_access ${CMAKE_CURRENT_SOURCE_DIR}/bench_remote_access.cpp)
#tc_add_executable(tc ${CMAKE_CURRENT_SOURCE_DIR}/tc.cpp)
#tc_add_executable(graph_compare ${CMAKE_CURRENT_SOURCE_DIR}/graph_compare.cpp)
#tc_add_executable(tc_naive ${CMAKE_CURRENT_SOURCE_DIR}/tc_naive.cpp)
