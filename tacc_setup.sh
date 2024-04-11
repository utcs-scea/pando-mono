# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

module load intel/19.1.1
module load impi/19.0.9
module load python3/3.9.7
. ./share/spack/setup-env.sh
spack compiler find
spack install -j$(nproc) gasnet@2023.3.0%intel@19.1.1.217 conduits=smp,mpi target=x86_64
