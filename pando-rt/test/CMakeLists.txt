# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

add_subdirectory(containers)
add_subdirectory(execution)
add_subdirectory(memory)
add_subdirectory(specific_storage)
add_subdirectory(sync)
add_subdirectory(utility)

if (PANDO_RT_BACKEND STREQUAL "PREP")
add_subdirectory(serialization)
endif()

pando_add_driver_test(test_index test_index.cpp)
pando_add_driver_test(test_init test_init.cpp)
pando_add_driver_test(test_return test_return.cpp)
