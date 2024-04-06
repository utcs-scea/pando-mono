# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys

def tablevalidation():
    for line in sys.stdin:
        line = line.replace('\n', '')
        arr = line.split(' ')
        for elem in arr:
            if elem.isdigit():
                if int(elem) > 9:
                    # Failure
                    return sys.exit(1)
    return "PASS"

# Call the function to process input from stdin
result = tablevalidation()
print(result)
