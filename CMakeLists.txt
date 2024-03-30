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

if(NOT DEFINED DISABLE_PANDORT_TESTS)
  set(ENABLE_PANDORT_TEST FALSE)
endif()

option(ENABLE_PANDORT_TESTS "Enable pando-rt to run tests" OFF)

if (NOT ENABLE_PANDORT_TESTS)
  set(BUILD_TESTING_SAVED "${BUILD_TESTING}")
  set(BUILD_TESTING OFF)
endif()
add_subdirectory(pando-rt)
if(NOT ENABLE_PANDORT_TESTS)
  set(BUILD_TESTING "${BUILD_TESTING_SAVED}")
endif()

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

# default build type
set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to default '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
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
        CXX_STANDARD                17
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
option(BUILD_TESTING "Build tests." ON)
if (BUILD_TESTING)
    include(cmake/PANDOTesting.cmake)
    enable_testing()
    add_subdirectory(test)
endif ()

# examples
option(BUILD_EXAMPLES "Build examples" ON)
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# documentation
option(BUILD_DOCS "Build documentation" ON)
if (BUILD_DOCS)
    add_subdirectory(docs)
endif ()
