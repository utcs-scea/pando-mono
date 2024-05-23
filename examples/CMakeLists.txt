# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(EXAMPLE_SOURCES
  benchmark_pointer
  benchmark_loadbalance
)

function(add_example EXAMPLES)
  foreach(EXAMPLE IN LISTS EXAMPLES)
    if (PANDO_RT_BACKEND STREQUAL "DRVX")
      add_library(${EXAMPLE} SHARED ${EXAMPLE}.cpp)
    else()
      add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    endif()
    target_link_libraries(${EXAMPLE} PUBLIC pando-lib-galois::pando-lib-galois)
    pando_compiler_warnings(${EXAMPLE})
    pando_compiler_options(${EXAMPLE})
  endforeach()
endfunction()

add_example("${EXAMPLE_SOURCES}")
