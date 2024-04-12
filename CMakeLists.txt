# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(
    pando-lib-galois
    VERSION       0.1.0
    DESCRIPTION   "Library for Galois data structures and functions on PANDO"
    HOMEPAGE_URL  "https://github.com/AMDResearch/PANDO-lib-gal"
    LANGUAGES     CXX
)

if(CMAKE_CXX_FLAGS)
    message(STATUS "Provided CXX Flags: " ${CMAKE_CXX_FLAGS})
endif()

option(ENABLE_GALOIS_TESTS "Enable GALOIS to run tests" OFF)
option(ENABLE_ROOT_TESTS "Enable ROOT to run tests" OFF)
option(ENABLE_INFLUENCE_MAX_TESTS "Enable Influence Maximization to run tests" OFF)
option(ENABLE_BFS_TESTS "Enable bfs to run tests" OFF)

if(ENABLE_GALOIS_TESTS OR ENABLE_ROOT_TESTS OR ENABLE_INFLUENCE_MAX_TESTS OR ENABLE_BFS_TESTS)
  enable_testing()
endif()

add_subdirectory(pando-rt)

# CMake modules
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/pando-rt/cmake/Modules)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# helper functions
include(cmake/PANDOCompilerOptions.cmake)

# disable in-source builds to avoid source tree corruption
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR
        "In-source builds are not supported. Please create a directory for build files and delete CMakeCache.txt and \
        the CMakefiles directory."
    )
endif()

#create the sanitize build type
set(CMAKE_CXX_FLAGS_ASAN
  "-O2 -fsanitize=address -DNDEBUG" CACHE STRING "Flags used by the C++ compiler during sanitizer builds"
  FORCE )
set(CMAKE_C_FLAGS_ASAN
  "-O2 -fsanitize=address -DNDEBUG" CACHE STRING "Flags used by the C compiler during sanitizer builds"
  FORCE )
set(CMAKE_EXE_LINKER_FLAGS_ASAN
  "" CACHE STRING "Flags used for linking binaries during sanitizer builds"
  FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_ASAN
  "" CACHE STRING "Flags used for linking shared libraries during sanitizer builds"
  FORCE )

MARK_AS_ADVANCED(
  CMAKE_CXX_FLAGS_ASAN
  CMAKE_C_FLAGS_ASAN
  CMAKE_EXE_LINKER_FLAGS_ASAN
  CMAKE_SHARED_LINKER_FLAGS_ASAN)

set(CMAKE_CXX_FLAGS_UBSAN
  "-O2 -fsanitize=undefined -DNDEBUG" CACHE STRING "Flags used by the C++ compiler during sanitizer builds"
  FORCE )
set(CMAKE_C_FLAGS_UBSAN
  "-O2 -fsanitize=undefined -DNDEBUG" CACHE STRING "Flags used by the C compiler during sanitizer builds"
  FORCE )
set(CMAKE_EXE_LINKER_FLAGS_UBSAN
  "" CACHE STRING "Flags used for linking binaries during sanitizer builds"
  FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_UBSAN
  "" CACHE STRING "Flags used for linking shared libraries during sanitizer builds"
  FORCE )

MARK_AS_ADVANCED(
  CMAKE_CXX_FLAGS_UBSAN
  CMAKE_C_FLAGS_UBSAN
  CMAKE_EXE_LINKER_FLAGS_UBSAN
  CMAKE_SHARED_LINKER_FLAGS_UBSAN)

set(CMAKE_CXX_FLAGS_LSAN
  "-O2 -fsanitize=leak -DNDEBUG" CACHE STRING "Flags used by the C++ compiler during sanitizer builds"
  FORCE )
set(CMAKE_C_FLAGS_LSAN
  "-O2 -fsanitize=leak -DNDEBUG" CACHE STRING "Flags used by the C compiler during sanitizer builds"
  FORCE )
set(CMAKE_EXE_LINKER_FLAGS_LSAN
  "" CACHE STRING "Flags used for linking binaries during sanitizer builds"
  FORCE )
set(CMAKE_SHARED_LINKER_FLAGS_LSAN
  "" CACHE STRING "Flags used for linking shared libraries during sanitizer builds"
  FORCE )

MARK_AS_ADVANCED(
  CMAKE_CXX_FLAGS_LSAN
  CMAKE_C_FLAGS_LSAN
  CMAKE_EXE_LINKER_FLAGS_LSAN
  CMAKE_SHARED_LINKER_FLAGS_LSAN)

# default build type
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "Sanitize" "Coverage")
set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to default '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # possible values of build type for cmake-gui
endif ()

# target
add_library(pando-lib-galois STATIC)
add_library(pando-lib-galois::pando-lib-galois ALIAS pando-lib-galois)

# headers
include(GenerateExportHeader)
generate_export_header(pando-lib-galois EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/pando-lib-galois/export.h)

target_include_directories(pando-lib-galois
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/pando-rt/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/pando-lib-galois>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-lib-galois/loops.hpp
)

# library properties
set_target_properties(pando-lib-galois
    PROPERTIES
        CXX_STANDARD                23
        CXX_STANDARD_REQUIRED       ON
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       hidden
        VISIBILITY_INLINES_HIDDEN   ON
        POSITION_INDEPENDENT_CODE   ON
        VERSION                     "${PROJECT_VERSION}"
        SOVERSION                   "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME                 pando-lib-galois
        OUTPUT_NAME                 pando-lib-galois
        PUBLIC_HEADER               "${PUBLIC_HEADERS}"
        # enable RPATH during both build and installation; use relative RPATH as well
        SKIP_BUILD_RPATH            FALSE
        BUILD_WITH_INSTALL_RPATH    FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH                 "\$ORIGIN"
        INSTALL_RPATH               "\$ORIGIN"
        # organize lib / bin in the build directory
        ARCHIVE_OUTPUT_DIRECTORY    ${PROJECT_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY    ${PROJECT_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY    ${PROJECT_BINARY_DIR}/bin
)

# sources
add_subdirectory(src)

# tests
include(cmake/PANDOTesting.cmake)
if (ENABLE_GALOIS_TESTS)
    add_subdirectory(test)
endif ()

# Microbenchmarks
option(BUILD_MICROBENCH "Build Microbenchmarks" OFF)
if (BUILD_MICROBENCH)
  add_subdirectory(microbench)
endif()

# examples
option(BUILD_EXAMPLES "Build examples" OFF)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# documentation
option(BUILD_DOCS "Build documentation" OFF)
if (BUILD_DOCS)
    add_subdirectory(docs)
endif ()

# workflows
option(BUILD_WORKFLOWS "Build Workflows" OFF)
if (BUILD_WORKFLOWS)
add_subdirectory(workflows)
endif()
