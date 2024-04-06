# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

#
# dependencies
#

include(CTest)

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
  set(PANDO_TEST_DISCOVERY_TIMEOUT "" CACHE STRING "Test discover timeout (secs)")
endif ()

set(PANDO_TEST_DEFAULT_TIMEOUT "120" CACHE STRING "Test timeout (secs)")

#
# functions
#

# Adds a source file as a GoogleTest based test
function(pando_add_test TARGET SOURCEFILE)
  add_executable(${TARGET} ${SOURCEFILE})
  target_link_libraries(${TARGET}
    PRIVATE
      GTest::gtest_main
      pando-rt::pando-rt
  )
  pando_compiler_options(${TARGET})
  pando_compiler_warnings(${TARGET})

  if (${PANDO_TEST_DISCOVERY})
    if (NOT DEFINED ${PANDO_TEST_DISCOVERY_TIMEOUT})
      # use default test discovery timeout
      gtest_discover_tests(${TARGET}
        PROPERTIES
          TIMEOUT ${PANDO_TEST_DEFAULT_TIMEOUT}
      )
    else ()
      gtest_discover_tests(${TARGET}
        DISCOVERY_TIMEOUT ${PANDO_TEST_DISCOVERY_TIMEOUT}
        PROPERTIES
          TIMEOUT ${PANDO_TEST_DEFAULT_TIMEOUT}
      )
    endif ()
  endif ()
endfunction()

# Adds a source file as a GoogleTest based test that uses the emulator driver
function(pando_add_driver_test TARGET SOURCEFILE)
  if (NOT DEFINED ${PANDO_TEST_DISCOVERY_TIMEOUT})
    set(DRIVER_DISCOVERY_TIMEOUT 15) # use 15s to avoid GASNet occasional init delays
  else ()
    set(DRIVER_DISCOVERY_TIMEOUT ${PANDO_TEST_DISCOVERY_TIMEOUT})
  endif ()

  if (PANDO_RT_BACKEND STREQUAL "PREP")
    if (${GASNet_CONDUIT} STREQUAL "smp")
      set(RUNNER_SCRIPT ${PROJECT_SOURCE_DIR}/../scripts/preprun.sh)
    elseif (${GASNet_CONDUIT} STREQUAL "mpi")
      set(RUNNER_SCRIPT ${PROJECT_SOURCE_DIR}/../scripts/preprun_mpi.sh)
    else ()
      message(FATAL_ERROR "No runner script for GASNet conduit ${GASNet_CONDUIT}")
    endif ()
  elseif (PANDO_RT_BACKEND STREQUAL "DRVX")
    set(RUNNER_SCRIPT ${PROJECT_SOURCE_DIR}/scripts/drvxrun.sh)
  else ()
    message(FATAL_ERROR "No runner script for backend ${PANDO_RT_BACKEND}")
  endif ()

  set(NUM_PXNS 2)
  set(NUM_CORES 4)

  pando_add_executable(${TARGET} ${PROJECT_SOURCE_DIR}/test/test_driver.cpp ${SOURCEFILE})

  if (PANDO_RT_BACKEND STREQUAL "PREP")
    target_link_libraries(${TARGET}
      PRIVATE
        GTest::gtest
    )
  elseif (PANDO_RT_BACKEND STREQUAL "DRVX")
    target_link_libraries(${TARGET} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,GTest::gtest>")
    set_target_properties(gtest PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # create a dummy executable as a different target but with the same name for ctest to discover the right test programs
    add_executable(${TARGET}-drvx ${PROJECT_SOURCE_DIR}/test/test_dummy.cpp)
    set_target_properties(${TARGET}-drvx PROPERTIES OUTPUT_NAME ${TARGET})

    # Robust method to bail on error: create regex variable to track failed tests via stdout/stderr rather than rely on return code of drvx-sst
    set(FAIL_REGEX "FAIL_REGULAR_EXPRESSION;Failed;FAIL_REGULAR_EXPRESSION;ERROR;FAIL_REGULAR_EXPRESSION;FAILED;")
  else()
    message(FATAL_ERROR "No test support for backend ${PANDO_RT_BACKEND}")
  endif()

  # use CROSSCOMPILING_EMULATOR to point to the driver script
  # it cannot be added to set_target_properties
  set_property(TARGET ${TARGET}
    PROPERTY CROSSCOMPILING_EMULATOR '${RUNNER_SCRIPT} -n ${NUM_PXNS} -c ${NUM_CORES}'
  )

  if (${PANDO_TEST_DISCOVERY})
    gtest_discover_tests(${TARGET}
      DISCOVERY_TIMEOUT ${DRIVER_DISCOVERY_TIMEOUT}
      PROPERTIES
        ${FAIL_REGEX}
        TIMEOUT ${PANDO_TEST_DEFAULT_TIMEOUT}
      )
  endif ()
endfunction()
