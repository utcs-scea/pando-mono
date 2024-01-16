# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include_guard()

option(PANDO_WERROR "Make all warnings into errors." ON)

# Default compiler options for targets
function(pando_compiler_options TARGET)
    set_target_properties(${TARGET}
        PROPERTIES
            CXX_STANDARD                17
            CXX_STANDARD_REQUIRED       ON
            CXX_EXTENSIONS              OFF
            CXX_VISIBILITY_PRESET       hidden
            VISIBILITY_INLINES_HIDDEN   ON
            POSITION_INDEPENDENT_CODE   ON
    )
    target_compile_features(${TARGET}
        PUBLIC
            cxx_std_17)
endfunction()

# Default compiler warnings for targets
function(pando_compiler_warnings TARGET)
    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic $<$<BOOL:${PANDO_WERROR}>:-Werror>>
    )
endfunction()

function(pando_add_executable TARGET)
    if (PANDO_RT_BACKEND STREQUAL "DRVX")
        add_library(${TARGET} SHARED ${ARGN})
        target_link_libraries(${TARGET} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-lib-galois::pando-lib-galois>")
    else()
        add_executable(${TARGET} ${ARGN})
        target_link_libraries(${TARGET} PRIVATE pando-lib-galois::pando-lib-galois)
    endif()
    pando_compiler_warnings(${TARGET})
    pando_compiler_options(${TARGET})
endfunction()
