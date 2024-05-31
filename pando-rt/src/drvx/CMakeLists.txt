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

find_package(Drvx REQUIRED)

#
# target
#

add_library(pando-rt-drvx)

target_include_directories(pando-rt-drvx
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/pando-rt>"
)

target_sources(pando-rt-drvx
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cores.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cp.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/drvx.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/index.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/log.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/status.hpp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cores.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cp.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/drvx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/index.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/status.cpp
)

pando_compiler_warnings(pando-rt-drvx)
pando_compiler_options(pando-rt-drvx)

set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_definitions(pando-rt-drvx
    PUBLIC
        "-DPANDO_RT_USE_BACKEND_DRVX=1"
)

if(PANDO_RT_DRVX_ENABLE_DMA STREQUAL "ON")
    target_compile_definitions(pando-rt-drvx
            PUBLIC
                "-DPANDO_RT_USE_DMA=1"
    )
endif()

if(PANDO_RT_DRVX_ENABLE_BYPASS STREQUAL "ON")
    target_compile_definitions(pando-rt-drvx
            PUBLIC
                "-DPANDO_RT_BYPASS=1"
    )
endif()


target_link_libraries(pando-rt-drvx
    PUBLIC
        spdlog::spdlog
    PRIVATE
        drvx::drvx
)
