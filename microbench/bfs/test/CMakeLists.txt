# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_driver_test_lib(test_sssp test_sssp.cpp pando-bfs::pando-bfs)
pando_add_bin_tag_test(DLCSR ${pando-bfs-galois_BINARY_DIR}../src/bfs "-d -n 8 -s 0 -s 1 -s 2 -s 3 -s 4 -s 5 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/quick.el ${pando-bfs_SOURCE_DIR}/ok/quick.el-8-0-1-2-3-4-5.ok)
pando_add_bin_tag_test(MDLCSR ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 8 -s 0 -s 1 -s 2 -s 3 -s 4 -s 5 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/quick.el ${pando-bfs_SOURCE_DIR}/ok/quick.el-8-0-1-2-3-4-5.ok)
pando_add_bin_tag_test(MDLCSR1024 ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 1024 -s 0 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el ${pando-bfs_SOURCE_DIR}/ok/rmat_571919_seed1_scale10_nV1024_nE10447.el.ok)
pando_add_bin_tag_test(MDLCSR2048 ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 2048 -s 0 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el ${pando-bfs_SOURCE_DIR}/ok/rmat_571919_seed1_scale11_nV2048_nE22601.el.ok)
pando_add_bin_tag_test(MDLCSR4096 ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 4096 -s 0 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el ${pando-bfs_SOURCE_DIR}/ok/rmat_571919_seed1_scale12_nV4096_nE48335.el.ok)
#pando_add_bin_tag_test(MDLCSR8192 ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 8192 -s 0 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el ${pando-bfs_SOURCE_DIR}/ok/rmat_571919_seed1_scale13_nV8192_nE102016.el.ok)
#pando_add_bin_tag_test(MDLCSR16384 ${pando-bfs-galois_BINARY_DIR}../src/bfs "-m -n 16384 -s 0 -f " ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el ${pando-bfs_SOURCE_DIR}/ok/rmat_571919_seed1_scale14_nV16384_nE213350.el.ok)
