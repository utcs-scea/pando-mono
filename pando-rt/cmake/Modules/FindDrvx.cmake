# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

#[=======================================================================[.rst:
FindDrvx
--------

Find the drvx library.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``drvx::drvx``,
if drvx has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  Drvx_INCLUDE_DIRS   - where to find drvx headers
  Drvx_LIBRARIES      - List of libraries when using drvx.
  Drvx_FOUND          - True if drvx found.

Hints
^^^^^

A user may set ``DRVX_ROOT`` to a drvx library installation
root to tell this module where to look.

#]=======================================================================]

if (DEFINED ENV{DRVX_ROOT})
  set(DRVX_ROOT "$ENV{DRVX_ROOT}")
endif ()

find_path(Drvx_INCLUDE_DIR
  NAMES
    DrvAPI.hpp
  PATH_SUFFIXES
    include
  HINTS
    ${DRVX_ROOT}
)

find_library(Drvx_LIBRARY
  NAMES
    drvapi
  PATH_SUFFIXES
    lib
    lib64
  HINTS
    ${DRVX_ROOT}
)
mark_as_advanced(Drvx_INCLUDE_DIR Drvx_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_Args(Drvx
  REQUIRED_VARS
    Drvx_INCLUDE_DIR
    Drvx_LIBRARY
)

if (Drvx_FOUND)

  set(Drvx_INCLUDE_DIRS ${Drvx_INCLUDE_DIR})

  if (NOT Drvx_LIBRARIES)
    set(Drvx_LIBRARIES ${Drvx_LIBRARY})
  endif ()

  if (NOT TARGET drvx::drvx)
    add_library(drvx::drvx UNKNOWN IMPORTED)
    set_target_properties(drvx::drvx
      PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES  "CXX"
        IMPORTED_LOCATION                  "${Drvx_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES      "${Drvx_INCLUDE_DIR}"
    )
  endif ()

endif ()
