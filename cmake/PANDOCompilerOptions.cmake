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
