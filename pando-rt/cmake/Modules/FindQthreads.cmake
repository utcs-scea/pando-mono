# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

#[=======================================================================[.rst:
FindQthreads
--------

Find the qthreads library.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``qthreads::qthreads``,
if qthreads has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  qthreads_INCLUDE_DIRS   - where to find qthreads headers
  qthreads_LIBRARIES      - List of libraries when using qthreads.
  qthreads_FOUND          - True if qthreads found.

Hints
^^^^^

A user may set ``QTHREADS_ROOT`` to a qthreads library installation
root to tell this module where to look.

#]=======================================================================]

if (DEFINED ENV{QTHREADS_ROOT})
  set(QTHREADS_ROOT "$ENV{QTHREADS_ROOT}")
endif ()

find_path(Qthreads_INCLUDE_DIR
  NAMES
    qthread/qthread.h
  PATH_SUFFIXES
    include
  HINTS
    ${QTHREADS_ROOT}
)

find_library(Qthreads_LIBRARY
  NAMES
    qthread
  PATH_SUFFIXES
    lib
    lib64
  HINTS
    ${QTHREADS_ROOT}
)

mark_as_advanced(Qthreads_INCLUDE_DIR Qthreads_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_Args(Qthreads
  REQUIRED_VARS
    Qthreads_INCLUDE_DIR
    Qthreads_LIBRARY
)

if (Qthreads_FOUND)

  set(Qthreads_INCLUDE_DIRS ${Qthreads_INCLUDE_DIR})

  if (NOT Qthreads_LIBRARIES)
    set(Qthreads_LIBRARIES ${Qthreads_LIBRARY})
  endif ()

  if (NOT TARGET qthreads::qthreads)
    add_library(qthreads::qthreads UNKNOWN IMPORTED)
    set_target_properties(qthreads::qthreads
      PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES  "C"
        IMPORTED_LOCATION                  "${Qthreads_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES      "${Qthreads_INCLUDE_DIR}"
    )
  endif ()

endif ()
