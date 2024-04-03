# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(
    pando-triangle-counting
    VERSION       0.2.0
    DESCRIPTION   "Triangle Counting Galois implementation for PANDO"
    HOMEPAGE_URL  "https://github.com/utcs-scea/pando-mono"
    LANGUAGES     CXX
)

install(TARGETS pando-lib-galois)

# CMake modules
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# helper functions
include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

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
add_library(pando-triangle-counting STATIC)
add_library(pando-triangle-counting::pando-triangle-counting ALIAS pando-triangle-counting)

# headers
include(GenerateExportHeader)
generate_export_header(pando-triangle-counting EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/pando-triangle-counting/export.h)

target_include_directories(pando-triangle-counting
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-rt_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# library properties
set_target_properties(pando-triangle-counting
    PROPERTIES
        CXX_STANDARD                17
        CXX_STANDARD_REQUIRED       ON
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       hidden
        VISIBILITY_INLINES_HIDDEN   ON
        POSITION_INDEPENDENT_CODE   ON
        VERSION                     "${PROJECT_VERSION}"
        SOVERSION                   "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME                 pando-triangle-counting
        OUTPUT_NAME                 pando-triangle-counting
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
option(MICROBENCH_TESTING "Testing for microbenchmarks" ON)
if (MICROBENCH_TESTING)
    include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOTesting.cmake)
    enable_testing()
    add_subdirectory(test)
endif ()

# documentation
option(BUILD_DOCS "Build documentation" ON)
if (BUILD_DOCS)
    add_subdirectory(docs)
endif ()
