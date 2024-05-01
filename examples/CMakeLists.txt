# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(EXAMPLE_SOURCES
  benchmark_pointer
)

function(add_example EXAMPLES)
    foreach(EXAMPLE IN LISTS EXAMPLES)
        message(${EXAMPLE})
        add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
        target_link_libraries(${EXAMPLE} PRIVATE pando-lib-galois::pando-lib-galois)
        pando_compiler_warnings(${EXAMPLE})
        pando_compiler_options(${EXAMPLE})
    endforeach()
endfunction()

add_example("${EXAMPLE_SOURCES}")
