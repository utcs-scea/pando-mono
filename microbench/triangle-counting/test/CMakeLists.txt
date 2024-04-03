# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

pando_add_driver_test(test_tc_naive test_tc_naive.cpp)
pando_add_driver_test(test_tc test_tc.cpp)
# TODO(Patrick) add argument to `pando_add_driver_test` for library like in scea/gls
target_link_libraries(test_tc_naive PRIVATE pando-triangle-counting::pando-triangle-counting)
target_link_libraries(test_tc PRIVATE pando-triangle-counting::pando-triangle-counting)

## binary tests
pando_add_bin_test(../src/tc_naive "-n 10 -f" ${PROJECT_SOURCE_DIR}/graphs/simple.el ${PROJECT_SOURCE_DIR}/ok/simple.el-10.ok)
pando_add_bin_test(../src/tc_naive "-n 8 -f" ${PROJECT_SOURCE_DIR}/graphs/scale3.el ${PROJECT_SOURCE_DIR}/ok/scale3.el-8.ok)
pando_add_bin_test(../src/tc_naive "-n 16 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_561919_seed1_scale4_nV16_nE65.el ${PROJECT_SOURCE_DIR}/ok/scale4.el-16.ok)
pando_add_bin_test(../src/tc_naive "-n 32 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_561919_seed1_scale5_nV32_nE153.el ${PROJECT_SOURCE_DIR}/ok/scale5.el-32.ok)
pando_add_bin_test(../src/tc_naive "-n 64 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale6_nV64_nE403.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale6_nV64_nE403.el-64.ok)
pando_add_bin_test(../src/tc_naive "-n 128 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale7_nV128_nE942.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale7_nV128_nE942.el-128.ok )
pando_add_bin_test(../src/tc_naive "-n 256 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale8_nV256_nE2144.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale8_nV256_nE2144.el-256.ok)
pando_add_bin_test(../src/tc_naive "-n 512 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale9_nV512_nE4736.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale9_nV512_nE4736.el-512.ok)
pando_add_bin_test(../src/tc_naive "-n 1024 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale10_nV1024_nE10447.el-1024.ok)
pando_add_bin_test(../src/tc_naive "-n 2048 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale11_nV2048_nE22601.el-2048.ok)
pando_add_bin_test(../src/tc_naive "-n 4096 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale12_nV4096_nE48335.el-4096.ok)
pando_add_bin_test(../src/tc_naive "-n 8192 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale13_nV8192_nE102016.el-8192.ok)
pando_add_bin_test(../src/tc_naive "-n 16384 -f" ${PROJECT_SOURCE_DIR}/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el
  ${PROJECT_SOURCE_DIR}/ok/rmat_571919_seed1_scale14_nV16384_nE213350.el-16384.ok)
