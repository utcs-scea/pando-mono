# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

function(pando_add_exec TARGET SOURCEFILE)
  add_executable(${TARGET} ${SOURCEFILE})
  target_link_libraries(${TARGET}
    PRIVATE
      pando-lib-galois::pando-lib-galois
  )
  pando_compiler_options(${TARGET})
  pando_compiler_warnings(${TARGET})
endfunction()

add_subdirectory(containers)
add_subdirectory(graphs)
add_subdirectory(loops)
add_subdirectory(sync)

pando_add_exec(import_csrlist import_csrlist.cpp)

pando_add_bin_test(import_csrlist "-n 10 -f" ${pando-lib-galois_SOURCE_DIR}/graphs/simple.el ${pando-lib-galois_SOURCE_DIR}/ok/simple.el-import-csrlist-10.ok)
