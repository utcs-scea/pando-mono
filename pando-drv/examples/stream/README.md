<!--
  ~ SPDX-License-Identifier: MIT
  ~ Copyright (c) 2023. University of Texas at Austin. All rights reserved.
  -->

# DRV_STREAM

## Notes
This version of stream benchmark is written for the PandoHammer SST model.
It uses Drv::API under the hood instead of loads/stores and relies on explicit index calculation
based on the tile number and number of threads per tile (hardcoded in the .cpp file).
The benchmark runs Triad() by default. You can uncomment the rest of the kernels if you wish to run them instead.

The code is based of the **reference stream benchmark** implementation and **NOT BabelStream!**

## Compile and run
Make sure your environment is set up correctly like for other Drv examples - boost, drvapi, sst, and other paths are set correctly (you might need to change couple of .mk files too).
```
make run
```
Use the provided example script to run in SST. Obviously you will have to use a valid SST configuration script and paths.
