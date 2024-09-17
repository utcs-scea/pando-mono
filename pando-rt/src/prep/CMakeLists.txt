# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${PROJECT_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)
include(FetchContent)

#
# dependencies
#

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.11.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal
    GIT_TAG        v1.3.2
)
set(JUST_INSTALL_CEREAL ON CACHE INTERNAL "")
FetchContent_MakeAvailable(cereal)

# setup GASNet with supported conduits
set(PANDO_PREP_GASNET_CONDUIT_LIST "smp" "mpi" "ibv")
set(PANDO_PREP_GASNET_CONDUIT smp CACHE STRING "PREP GASNet conduit")
set_property(CACHE PANDO_PREP_GASNET_CONDUIT PROPERTY STRINGS ${PANDO_PREP_GASNET_CONDUIT_LIST})
message(STATUS "PREP GASNet conduit: ${PANDO_PREP_GASNET_CONDUIT}")
set(GASNet_CONDUIT "${PANDO_PREP_GASNET_CONDUIT}")
set(GASNet_THREADING "par")
find_package(GASNet REQUIRED)

find_package(Qthreads REQUIRED)

#
# target
#

add_library(pando-rt-prep)

target_include_directories(pando-rt-prep
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/pando-rt>"
)

target_sources(pando-rt-prep
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/config.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cores.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data_type.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/hart_context.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/index.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/log.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/memory.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/nodes.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/status.hpp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cores.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hart_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/index.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/nodes.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/status.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/termination.cpp
)

if(NOT PANDO_RT_ENABLE_MEM_TRACE STREQUAL "OFF")
    target_sources(pando-rt-prep
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/memtrace_log.hpp>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/memtrace_log.cpp)

    if (PANDO_RT_ENABLE_MEM_TRACE STREQUAL "INTER-PXN")
        target_compile_definitions(pando-rt-prep
            PUBLIC
                "-DPANDO_RT_TRACE_MEM_PREP=1"
                "-DPANDO_MEM_TRACE_OR_STAT=1")
    elseif (PANDO_RT_ENABLE_MEM_TRACE STREQUAL "ALL")
        target_compile_definitions(pando-rt-prep
            PUBLIC
                "-DPANDO_RT_TRACE_MEM_PREP=2"
                "-DPANDO_MEM_TRACE_OR_STAT=1")
    else ()
        message(FATAL_ERROR "PANDO_RT_ENABLE_MEM_TRACE must be OFF, INTER-PXN or ALL.")
    endif()
endif()

if(PANDO_RT_ENABLE_MEM_STAT STREQUAL "ON")
    target_sources(pando-rt-prep
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/memtrace_stat.hpp>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/memtrace_log.hpp>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/memtrace_stat.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/memtrace_log.cpp)

    target_compile_definitions(pando-rt-prep
            PUBLIC
                "-DPANDO_RT_ENABLE_MEM_STAT=1"
                "-DPANDO_MEM_TRACE_OR_STAT=1"
    )
endif()

pando_compiler_warnings(pando-rt-prep)
pando_compiler_options(pando-rt-prep)

target_compile_definitions(pando-rt-prep
    PUBLIC
        "-DCEREAL_THREAD_SAFE=1"
        "-DPANDO_RT_USE_BACKEND_PREP=1"
)

target_link_libraries(pando-rt-prep
    PUBLIC
        cereal::cereal
        spdlog::spdlog
    PRIVATE
        GASNet::GASNet
        qthreads::qthreads
)
