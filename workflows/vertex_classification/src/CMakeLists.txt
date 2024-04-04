# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${PROJECT_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-wf1-galois target
#

target_sources(pando-wf1-galois
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/import.cpp
)

pando_compiler_options(pando-wf1-galois)
pando_compiler_warnings(pando-wf1-galois)

# link pando-lib-galois
target_include_directories(pando-wf1-galois
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/pando-rt/include
        ${CMAKE_CURRENT_BINARY_DIR}/../Galois/include
)

target_link_libraries(pando-wf1-galois
    PUBLIC
        pando-lib-galois::pando-lib-galois
)

#if (PANDO_RT_BACKEND STREQUAL "DRVX")
#    add_library(wf1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
#    #add_library(mhr SHARED ${CMAKE_CURRENT_SOURCE_DIR}/mhr_main.cpp)
#    add_library(wf1-lp SHARED ${CMAKE_CURRENT_SOURCE_DIR}/lp.cpp)
#else()
#    add_executable(wf1 ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
#    #add_executable(mhr ${CMAKE_CURRENT_SOURCE_DIR}/mhr_main.cpp)
#    add_executable(wf1-lp ${CMAKE_CURRENT_SOURCE_DIR}/lp.cpp)
#endif()
#
#target_include_directories(wf1
#    PUBLIC
#        ${CMAKE_CURRENT_BINARY_DIR}/../include
#)
#target_link_libraries(wf1
#    PUBLIC
#        pando-wf1-galois::pando-wf1-galois
#)
##target_include_directories(mhr
##    PUBLIC
##        ${CMAKE_CURRENT_BINARY_DIR}/../include
##)
##target_link_libraries(mhr
##    PUBLIC
##        pando-wf1-galois::pando-wf1-galois
##)
#target_include_directories(wf1-lp
#    PUBLIC
#        ${CMAKE_CURRENT_BINARY_DIR}/../include
#)
#target_link_libraries(wf1-lp
#    PUBLIC
#        pando-wf1-galois::pando-wf1-galois
#)
