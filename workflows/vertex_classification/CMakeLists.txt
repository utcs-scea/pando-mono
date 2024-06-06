# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(
    pando-wf1
    VERSION       0.1.0
    DESCRIPTION   "Workflow 1 Galois implementation for PANDO"
    LANGUAGES     CXX
)

# CMake modules
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
pando_add_library(pando-wf1)

# headers
include(GenerateExportHeader)
generate_export_header(pando-wf1 EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/pando-wf1/export.h)

target_include_directories(pando-wf1
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-lib-galois_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${pando-rt_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/pando-wf1>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# library properties
set_target_properties(pando-wf1
    PROPERTIES
        CXX_STANDARD                23
        CXX_STANDARD_REQUIRED       ON
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       hidden
        VISIBILITY_INLINES_HIDDEN   ON
        POSITION_INDEPENDENT_CODE   ON
        VERSION                     "${PROJECT_VERSION}"
        SOVERSION                   "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME                 pando-wf1
        OUTPUT_NAME                 pando-wf1
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
    include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOTesting.cmake)
    enable_testing()
    # add_subdirectory(test)
endif ()
