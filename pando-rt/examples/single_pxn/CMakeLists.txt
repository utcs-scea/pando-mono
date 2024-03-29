# SPDX-License-Identifier: MIT
### Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved. ###

set(SPXN_EXAMPLE_SOURCES
    onecore_increment_value
    onecore_scatter_values
    pass_value_in_twocores
    pass_value_in_allcores
    serial_tasks
    twocores_scatter_values
)

add_example("${SPXN_EXAMPLE_SOURCES}")
