# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_driver_test_lib(test_sssp test_sssp.cpp pando-bfs::pando-bfs)
pando_add_bin_tag_test(DACSR ${pando-bfs-galois_BINARY_DIR}../src/bfs "-n 8 -s 0 -s 1 -s 2 -s 3 -s 4 -s 5 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/quick.el ${pando-bfs_SOURCE_DIR}/ok/quick.el-8-0-1-2-3-4-5.ok)
pando_add_bin_tag_test(DLCSR ${pando-bfs-galois_BINARY_DIR}../src/bfs "-d -n 8 -s 0 -s 1 -s 2 -s 3 -s 4 -s 5 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/quick.el ${pando-bfs_SOURCE_DIR}/ok/quick.el-8-0-1-2-3-4-5.ok)
