# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

# Running on a compute node
module load tacc-apptainer/1.2.2
make apptainer
. /dependencies/spack/share/spack/setup-env.sh && spack load openmpi && spack load gasnet

export PANDO_RT_ENABLE_MEM_STAT=ON
PANDO_RT_ENABLE_MEM_STAT=ON BUILD_MICROBENCH=ON make setup && make -C dockerbuild
