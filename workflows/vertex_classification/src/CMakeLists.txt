# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

include(${pando-lib-galois_SOURCE_DIR}/cmake/PANDOCompilerOptions.cmake)

#
# pando-wf1-galois target
#

target_sources(pando-wf1
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/import.cpp
)


if (PANDO_RT_BACKEND STREQUAL "DRVX")
    add_library(wf1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    # add_library(mhr SHARED ${CMAKE_CURRENT_SOURCE_DIR}/mhr_main.cpp)
    # add_library(wf1-lp SHARED ${CMAKE_CURRENT_SOURCE_DIR}/lp.cpp)
    target_link_libraries(wf1
        PUBLIC
            "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-wf1::pando-wf1>"
    )
    # target_link_libraries(mhr
    #     PUBLIC
    #         "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-wf1::pando-wf1>"
    # )
    # target_link_libraries(wf1-lp
    #     PUBLIC
    #         "$<LINK_LIBRARY:WHOLE_ARCHIVE,pando-wf1::pando-wf1>"
    # )
else()
    add_executable(wf1 ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
    # add_executable(mhr ${CMAKE_CURRENT_SOURCE_DIR}/mhr_main.cpp)
    # add_executable(wf1-lp ${CMAKE_CURRENT_SOURCE_DIR}/lp.cpp)
    target_link_libraries(wf1
        PUBLIC
            pando-wf1::pando-wf1
    )
    # target_link_libraries(mhr
    #     PUBLIC
    #         pando-wf1::pando-wf1
    # )
    # target_link_libraries(wf1-lp
    #     PUBLIC
    #         pando-wf1::pando-wf1
    # )
endif()

target_include_directories(wf1
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../include
)
# target_include_directories(mhr
#     PUBLIC
#         ${CMAKE_CURRENT_BINARY_DIR}/../include
# )
# target_include_directories(wf1-lp
#     PUBLIC
#         ${CMAKE_CURRENT_BINARY_DIR}/../include
# )

##target_include_directories(mhr
##    PUBLIC
##        ${CMAKE_CURRENT_BINARY_DIR}/../include
##)
##target_link_libraries(mhr
##    PUBLIC
##        pando-wf1-galois::pando-wf1-galois
##)
