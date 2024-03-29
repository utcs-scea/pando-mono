# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(EXAMPLE_SOURCES
    gups_no_tasks
    gups
    helloworld
    memory_resource
    stack_sizes
)

function(add_example EXAMPLES)
    foreach(EXAMPLE IN LISTS EXAMPLES)
        pando_add_executable(${EXAMPLE} ${EXAMPLE}.cpp)
    endforeach()
endfunction()

add_example("${EXAMPLE_SOURCES}")
add_subdirectory(single_pxn)
add_subdirectory(multi_pxns)
