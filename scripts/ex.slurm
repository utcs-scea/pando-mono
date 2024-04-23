#!/bin/bash
#----------------------------------------------------
#  -- Copy/edit this script as desired.  Launch by executing
#     "sbatch milan.serial.slurm" on a Lonestar6 login node.
#
#  -- Serial codes run on a single node (upper case N = 1).
#       A serial code ignores the value of lower case n,
#       but slurm needs a plausible value to schedule the job.
#
#  -- Use TACC's launcher utility to run multiple serial
#       executables at the same time, execute "module load launcher"
#       followed by "module help launcher".
#----------------------------------------------------
#SBATCH -J example           # Job name
#SBATCH -o example.o%j       # Name of stdout output file
#SBATCH -e example.e%j       # Name of stderr error file
#SBATCH -p development        # Queue (partition) name
#SBATCH -N 1               # Total # of nodes (must be 1 for serial)
#SBATCH -n 1               # Total # of mpi tasks (should be 1 for serial)
#SBATCH -t 01:30:00        # Run time (hh:mm:ss)
#SBATCH --mail-type=none
#SBATCH --mail-user=prachi.ingle@utexas.edu

# Any other commands must follow all #SBATCH directives...
module load tacc-apptainer/1.2.2
make apptainer
. /dependencies/spack/share/spack/setup-env.sh && spack load openmpi && spack load gasnet

export PANDO_RT_ENABLE_MEM_STAT=ON
PANDO_RT_ENABLE_MEM_STAT=ON BUILD_MICROBENCH=ON make setup && make -C dockerbuild

export PANDO_RT_ENABLE_MEM_STAT=ON
PANDO_RT_ENABLE_MEM_STAT=ON BUILD_MICROBENCH=ON make setup && make -C dockerbuild

MPIRUN_CMD="mpirun -np %N -host localhost:32 --mca btl self,vader,tcp --map-by slot:PE=16 --use-hwthread-cpus %P %A" PANDO_PREP_MAIN_NODE=17179869184 ./scripts/preprun_mpi.sh -c 32 -n 2 ./dockerbuild/microbench/triangle-counting/src/tc -i graphs/rmat_571919_seed1_scale5_nV32_nE153.el -v 32 -c 0 -g 0
