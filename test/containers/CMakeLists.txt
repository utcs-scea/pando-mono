# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_driver_test(test_dist_array test_dist_array.cpp)
pando_add_driver_test(test_hashtable test_hashtable.cpp)
pando_add_driver_test(test_per_thread test_per_thread.cpp)
pando_add_driver_test(test_stack test_stack.cpp)

if (NOT PANDO_RT_BACKEND STREQUAL "DRVX")
  pando_add_driver_test(test_host_indexed_map test_host_indexed_map.cpp)
endif()
