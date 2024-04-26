# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${PROJECT_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-rt target
#

target_include_directories(pando-rt
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include/pando-rt>"
)

target_sources(pando-rt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/init.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/queue.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/specific_storage.hpp>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/start.hpp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/allocate_memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/atomic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/execute_on.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/global_ptr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/init.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/locality.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory_info.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/memory_resources.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/request.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/specific_storage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/start.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/stdlib.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/wait.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/program_stage.cpp
)

if(PANDO_RT_ENABLE_MEM_STAT STREQUAL "ON")
    target_sources(pando-rt
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tracing.cpp
    )
endif()

pando_compiler_warnings(pando-rt)
pando_compiler_options(pando-rt)

#
# backend selection
#

set(PANDO_RT_BACKEND_LIST "PREP" "DRVX" "DRVR")
set(PANDO_RT_BACKEND PREP CACHE STRING "pando-rt backend")
set_property(CACHE PANDO_RT_BACKEND PROPERTY STRINGS ${PANDO_RT_BACKEND_LIST})
message(STATUS "pando-rt backend: ${PANDO_RT_BACKEND}")

if (PANDO_RT_BACKEND STREQUAL "PREP")
    add_subdirectory(prep)

    # propagating public definitions of pando-rt-prep to pando-rt
    get_target_property(PREP_PUBLIC_COMPILE_DEFINITIONS
        pando-rt-prep INTERFACE_COMPILE_DEFINITIONS)
    target_compile_definitions(pando-rt
        PUBLIC
            ${PREP_PUBLIC_COMPILE_DEFINITIONS}
    )

    target_link_libraries(pando-rt
        PUBLIC
            # adding cereal as a public dependency to support serialization of data types for
            # communication from ROOT users
            cereal::cereal
        PRIVATE
            pando-rt-prep
    )
elseif (PANDO_RT_BACKEND STREQUAL "DRVX")
    add_subdirectory(drvx)

    # propagating public definitions of pando-rt-prep to pando-rt
    get_target_property(DRVX_PUBLIC_COMPILE_DEFINITIONS
        pando-rt-drvx INTERFACE_COMPILE_DEFINITIONS)
    target_compile_definitions(pando-rt
        PUBLIC
            ${DRVX_PUBLIC_COMPILE_DEFINITIONS}
    )

    target_link_libraries(pando-rt
        PRIVATE
            pando-rt-drvx
    )

elseif (PANDO_RT_BACKEND STREQUAL "DRVR")
    message(FATAL_ERROR "Backend DRVR is not supported yet")

    target_compile_definitions(pando-rt
        PUBLIC
            "-DPANDO_RT_USE_BACKEND_DRVR=1"
    )
else ()
    message(FATAL_ERROR "Unknown backend ${PANDO_RT_BACKEND}")
endif ()
