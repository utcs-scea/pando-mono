# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

#
# dependencies
#

include(FetchContent)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

include(${PROJECT_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# options
#

option(PANDO_TEST_DISCOVERY "Enable test discovery for use with ctest." ON)
if (${PANDO_TEST_DISCOVERY})
  set(PANDO_TEST_DISCOVERY_TIMEOUT "" CACHE STRING "GoogleTest test discover timeout in seconds")
endif ()

#
# functions
#

# Adds a source file as a GoogleTest based test
function(pando_add_test TARGET SOURCEFILE)
  add_executable(${TARGET} ${SOURCEFILE})
  target_link_libraries(${TARGET}
    PRIVATE
      GTest::gtest_main
      pando-lib-galois::pando-lib-galois
  )
  pando_compiler_options(${TARGET})
  pando_compiler_warnings(${TARGET})

  if (${PANDO_TEST_DISCOVERY})
    if (NOT DEFINED ${PANDO_TEST_DISCOVERY_TIMEOUT})
      # use default test discovery timeout
      gtest_discover_tests(${TARGET})
    else ()
      gtest_discover_tests(${TARGET}
        DISCOVERY_TIMEOUT ${PANDO_TEST_DISCOVERY_TIMEOUT}
      )
    endif ()
  endif ()
endfunction()

# Adds a source file as a GoogleTest based test that uses the emulator driver
function(pando_add_driver_test TARGET SOURCEFILE)
  if (NOT DEFINED ${PANDO_TEST_DISCOVERY_TIMEOUT})
    set(DRIVER_DISCOVERY_TIMEOUT 15) # use 15s to avoid GASNet smp occasional init delays
  else ()
    set(DRIVER_DISCOVERY_TIMEOUT ${PANDO_TEST_DISCOVERY_TIMEOUT})
  endif ()

  if (${GASNet_CONDUIT} STREQUAL "smp")
    set(DRIVER_SCRIPT ${PROJECT_SOURCE_DIR}/pando-rt/scripts/preprun.sh)
  elseif (${GASNet_CONDUIT} STREQUAL "mpi")
    set(DRIVER_SCRIPT ${PROJECT_SOURCE_DIR}/pando-rt/scripts/preprun_mpi.sh)
  else ()
    message(FATAL_ERROR "No runner script for GASNet conduit ${GASNet_CONDUIT}")
  endif ()
  set(NUM_PXNS 2)
  set(NUM_CORES 4)

  add_executable(${TARGET} ${PROJECT_SOURCE_DIR}/test/test_driver.cpp ${SOURCEFILE})
  target_link_libraries(${TARGET}
    PRIVATE
      GTest::gtest
      pando-lib-galois::pando-lib-galois
  )
  pando_compiler_options(${TARGET})
  pando_compiler_warnings(${TARGET})

  # use CROSSCOMPILING_EMULATOR to point to the driver script
  # it cannot be added to set_target_properties
  set_property(TARGET ${TARGET}
    PROPERTY CROSSCOMPILING_EMULATOR '${DRIVER_SCRIPT} -n ${NUM_PXNS} -c ${NUM_CORES}'
  )

  if (${PANDO_TEST_DISCOVERY})
    gtest_discover_tests(${TARGET}
      DISCOVERY_TIMEOUT ${PANDO_TEST_DISCOVERY_TIMEOUT}
    )
  endif ()
endfunction()

function(pando_add_bin_test TARGET ARGS INPUTFILE OKFILE)
  if (${GASNet_CONDUIT} STREQUAL "smp")
    set(DRIVER_SCRIPT ${PROJECT_SOURCE_DIR}/pando-rt/scripts/preprun.sh)
  elseif (${GASNet_CONDUIT} STREQUAL "mpi")
    set(DRIVER_SCRIPT ${PROJECT_SOURCE_DIR}/pando-rt/scripts/preprun_mpi.sh)
  else ()
    message(FATAL_ERROR "No runner script for GASNet conduit ${GASNet_CONDUIT}")
  endif ()

  set(NUM_PXNS 2)
  set(NUM_CORES 4)

  add_test(NAME ${TARGET}-${INPUTFILE}-${OKFILE}
    COMMAND bash -c "diff -Z <(${DRIVER_SCRIPT} -n ${NUM_PXNS} -c ${NUM_CORES} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} ${ARGS} ${INPUTFILE}) ${OKFILE}")
endfunction()
