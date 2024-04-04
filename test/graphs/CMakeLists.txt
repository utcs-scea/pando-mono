# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_driver_test(test_dist_array_csr test_dist_array_csr.cpp)
pando_add_driver_test(test_local_csr  test_local_csr.cpp)
pando_add_driver_test(test_dist_local_csr test_dist_local_csr.cpp)
pando_add_driver_test(test_mirror_dist_local_csr test_mirror_dist_local_csr.cpp)
pando_add_driver_test(test_map_exchange test_map_exchange.cpp)
