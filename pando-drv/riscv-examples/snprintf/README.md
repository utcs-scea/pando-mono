<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

```
SPDX-License-Identifier: MIT
Copyright (c) 2023 University of Washington
```

# Test Description
This tests two features.
1. Makes sure the cstdio call `snprintf()` can print integers in decimal and hexadecimal format and null terminate the output string.
2. That the system call `write()` works properly and indeed prints a string to the termainal.


Note that this test runs just one core and one thread.

```
x = 0, y = 1, z = 0x2
Simulation is complete, simulated time: 9.352 us
```
