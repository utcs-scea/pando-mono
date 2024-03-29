# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# SPDX-License-Identifier: MIT
# Copyright (c) 2024 University of Washington
import os
import sys


parameters = sys.argv[1:]
parameters_arg_str = ','.join([
    "[{}]".format(p) for p in parameters
])

makefile_str = """####################
# CHANGE ME: TESTS #
####################
# TESTS += $(call test-name,{})

""".format(
    parameters_arg_str
)

print(makefile_str)
