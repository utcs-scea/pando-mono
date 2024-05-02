# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(EXAMPLE_SOURCES
  benchmark_pointer
)

function(add_example EXAMPLES)
    foreach(EXAMPLE IN LISTS EXAMPLES)
      pando_add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    endforeach()
endfunction()

add_example("${EXAMPLE_SOURCES}")
