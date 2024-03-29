# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(MPXN_EXAMPLE_SOURCES
    onecore_scatter_diffdim_arrays_values
    onecore_scatter_samedim_arrays_values
    onecore_perpxn_reduce_values
    onecore_perpxn_reducebroadcast_values
    allcores_reducebroadcast_values
    allcores_reduce_values)
add_example("${MPXN_EXAMPLE_SOURCES}")
