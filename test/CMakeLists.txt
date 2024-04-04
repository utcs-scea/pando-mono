# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

function(pando_add_exec TARGET SOURCEFILE)
  if (PANDO_RT_BACKEND STREQUAL "DRVX")
    pando_add_executable(${TARGET} ${SOURCEFILE})
    target_link_libraries(${TARGET} PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-lib-galois::pando-lib-galois>")
  else()
    pando_add_executable(${TARGET} ${SOURCEFILE})
    target_link_libraries(${TARGET} PUBLIC pando-lib-galois::pando-lib-galois)
  endif()
endfunction()

pando_add_driver_test(fail_tests fail_tests.cpp)

add_subdirectory(containers)
add_subdirectory(graphs)
add_subdirectory(import)
add_subdirectory(loops)
add_subdirectory(sync)
add_subdirectory(sorts)
add_subdirectory(utility)

pando_add_exec(import_csrlist import_csrlist.cpp)
pando_add_exec(import_dirOptCsrList import_dirOptCsrList.cpp)
pando_add_exec(import_ifstream import_ifstream.cpp)

pando_add_bin_test(import_dirOptCsrList "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/repeats.el
  ${pando-lib-galois_SOURCE_DIR}/ok/repeats.el-import-dirOptCsrList-10.ok)

if (NOT PANDO_RT_BACKEND STREQUAL "DRVX") # for speed reasons

pando_add_bin_test(import_dirOptCsrList "-n 1024 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el
  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale10_nV1024_nE10447.el-import-dirOptCsrList-1024.ok)
pando_add_bin_test(import_dirOptCsrList "-n 2048 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el
  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale11_nV2048_nE22601.el-import-dirOptCsrList-2048.ok)
pando_add_bin_test(import_dirOptCsrList "-n 4096 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el
  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale12_nV4096_nE48335.el-import-dirOptCsrList-4096.ok)

endif()
#pando_add_bin_test(import_dirOptCsrList "-n 8192 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale13_nV8192_nE102016.el-import-dirOptCsrList-8192.ok)
#pando_add_bin_test(import_dirOptCsrList "-n 16384 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale14_nV16384_nE213350.el-import-dirOptCsrList-16384.ok)
#pando_add_bin_test(import_dirOptCsrList "-n 32768 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale15_nV32768_nE441929.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale15_nV32768_nE441929.el-import-dirOptCsrList-32768.ok)
#pando_add_bin_test(import_dirOptCsrList "-n 65536 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale16_nV65536_nE909846.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale16_nV65536_nE909846.el-import-dirOptCsrList-65536.ok)
#pando_add_bin_test(import_dirOptCsrList "-n 131072 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale17_nV131072_nE1864704.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale17_nV131072_nE1864704.el-import-dirOptCsrList-131072.ok)
#pando_add_bin_test(import_dirOptCsrList "-n 262144 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/rmat_571919_seed1_scale18_nV262144_nE3806162.el
#  ${pando-lib-galois_SOURCE_DIR}/ok/rmat_571919_seed1_scale18_nV262144_nE3806162.el-import-dirOptCsrList-262144.ok)
pando_add_bin_test(import_csrlist "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/simple.el ${pando-lib-galois_SOURCE_DIR}/ok/simple.el-import-csrlist-10.ok)
pando_add_bin_test(import_ifstream "-f" ${pando-lib-galois_SOURCE_DIR}/ok/ifstream.ok ${pando-lib-galois_SOURCE_DIR}/ok/ifstream.ok)
pando_add_bin_python_test(import_csrlist "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/simple.el ${pando-lib-galois_SOURCE_DIR}/scripts/dummy_validate.py)
