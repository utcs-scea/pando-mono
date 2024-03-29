# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

pando_add_driver_test(test_address_translation test_address_translation.cpp)
pando_add_driver_test(test_align test_align.cpp)
pando_add_driver_test(test_allocate_memory test_allocate_memory.cpp)
pando_add_driver_test(test_bump_resource test_bump_resource.cpp)
pando_add_driver_test(test_freelist_resource test_freelist_resource.cpp)
pando_add_driver_test(test_global_ptr test_global_ptr.cpp)
pando_add_driver_test(test_global_ref test_global_ref.cpp)
pando_add_driver_test(test_l2sp_memory_resource test_l2sp_memory_resource.cpp)
pando_add_driver_test(test_main_memory_resource test_main_memory_resource.cpp)
pando_add_driver_test(test_memory_guard test_memory_guard.cpp)
pando_add_driver_test(test_memory_info test_memory_info.cpp)
pando_add_driver_test(test_slab_resource test_slab_resource.cpp)
