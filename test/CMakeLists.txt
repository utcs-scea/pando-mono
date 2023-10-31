### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

add_subdirectory(containers)
add_subdirectory(graphs)
add_subdirectory(loops)

pando_add_test(test_example test.cpp)
pando_add_driver_test(test_driver_example test2.cpp)
