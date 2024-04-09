# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_exec(mirror_master_table test_mirror_master_table.cpp)

pando_add_bin_python_test(mirror_master_table "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/repeats.el ${pando-lib-galois_SOURCE_DIR}/test/python/mirror_master_validate.py)
pando_add_bin_python_test(mirror_master_table "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/simple.el ${pando-lib-galois_SOURCE_DIR}/test/python/mirror_master_validate.py)


if (NOT PANDO_RT_BACKEND STREQUAL "DRVX") # for speed reasons
pando_add_bin_python_test(mirror_master_table  "-n 1024 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el ${pando-lib-galois_SOURCE_DIR}/test/python/mirror_master_validate.py)
endif()
