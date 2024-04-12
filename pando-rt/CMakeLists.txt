# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(
    pando-rt
    VERSION       0.1.0
    DESCRIPTION   "PANDO Runtime System"
    HOMEPAGE_URL  "https://github.com/AMDResearch/pando-rt/"
    LANGUAGES     CXX C
)

# CMake modules
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake/Modules)
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

# check if it is built as part of a different project
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" is_top_level)
if (NOT "${is_top_level}")
    message(STATUS "Top-level project is ${CMAKE_PROJECT_NAME}")
endif ()

# default build type
set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to default '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# target
add_library(pando-rt STATIC)
add_library(pando-rt::pando-rt ALIAS pando-rt)

# headers
include(GenerateExportHeader)
generate_export_header(pando-rt EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/pando-rt/export.h)

if(PANDO_RT_BACKEND STREQUAL "PREP" AND PANDO_RT_ENABLE_MEM_STAT STREQUAL "ON")
    set(PANDO_STAT_TRACE_ENABLE 1)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/tracing.hpp.in  ${CMAKE_CURRENT_BINARY_DIR}/include/pando-rt/tracing.hpp @ONLY)

target_include_directories(pando-rt
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/pando-rt>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/containers/array.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/containers/vector.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/bulk_execute_on.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/execute_on_wait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/execute_on.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/request.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/result_storage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/execution/task.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/address_map.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/address_translation.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/allocate_memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/bump_memory_resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/common_memory_resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/freelist_memory_resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/global_ptr_fwd.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/global_ptr.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/memory_guard.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/memory_info.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/memory_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory/slab_memory_resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/serialization/archive.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/sync/atomic.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/sync/future.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/sync/mutex.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/sync/notification.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/sync/wait.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/utility/bit_manip.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/utility/function.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/utility/math.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/locality.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/main.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory_resource.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/memory.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/pando-rt.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/specific_storage.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/status.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/stddef.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/pando-rt/stdlib.hpp
)

# library properties
set_target_properties(pando-rt
    PROPERTIES
        CXX_STANDARD                23
        CXX_STANDARD_REQUIRED       ON
        CXX_EXTENSIONS              OFF
        CXX_VISIBILITY_PRESET       hidden
        VISIBILITY_INLINES_HIDDEN   ON
        POSITION_INDEPENDENT_CODE   ON
        VERSION                     "${PROJECT_VERSION}"
        SOVERSION                   "${PROJECT_VERSION_MAJOR}"
        EXPORT_NAME                 pando-rt
        OUTPUT_NAME                 pando-rt
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
if (ENABLE_ROOT_TESTS)
    include(cmake/PANDOTesting.cmake)
    enable_testing()
    add_subdirectory(test)
endif ()

# examples
if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# documentation
if (BUILD_DOCS)
    add_subdirectory(docs)
endif ()
