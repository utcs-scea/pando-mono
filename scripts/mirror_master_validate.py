# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys

def tablevalidation():
    dic = {}
    for line in sys.stdin:
        parts = line.strip().split(', ')
        if len(parts) != 3:
            continue

        operation, key, value = parts

        if operation == "SET":
            dic[key] = value
        elif operation == "FALSE":
            if key in dic and dic[key] == value:
                sys.exit(1)
        elif operation == "TRUE":
            if key not in dic or dic[key] != value:
                sys.exit(1)
    return "PASS"

# Call the function to process input from stdin
result = tablevalidation()
print(result)
