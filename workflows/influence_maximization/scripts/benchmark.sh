#!/bin/bash
# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

RUN_SCRIPT="${RUN_SCRIPT:-scripts/run.sh}"
export LAUNCH_SCRIPT="${LAUNCH_SCRIPT:-gasnetrun_mpi}"
export EXE="${EXE:-dockerbuild/src/wf4}"
export SEED="${SEED:-9801}"
export DISABLE_K2="${DISABLE_K2:-1}"
export DISABLE_K3="${DISABLE_K3:-1}"
export DISABLE_K4="${DISABLE_K4:-1}"

export HOST_THREADS="${HOST_THREADS:-16}"
export HOSTS="${HOSTS:-1}"
export PROCS="${PROCS:-8}"
export CORES="${CORES:-4}"
export HARTS="${HARTS:-16}"
export STACK_SIZE="${STACK_SIZE:-8192}"
# 33MB Pod memory size by default
export POD_MEMORY_SIZE="${POD_MEMORY_SIZE:-33554432}"
# 96GB Main memory size by default
export MAIN_MEMORY_SIZE="${MAIN_MEMORY_SIZE-103079215104}"

SCALE_n1_K="10"
SCALE_n1_RRR_SETS="100000"
SCALE_n1_COMMERCIAL_FILE="graphs/scale-1/commercial.csv"
SCALE_n1_CYBER_FILE="graphs/scale-1/cyber.csv"
SCALE_n1_SOCIAL_FILE="graphs/scale-1/social.csv"
SCALE_n1_USES_FILE="graphs/scale-1/uses.csv"
SCALE_n1_LOGFILE="data/scale-1_run.txt"

SCALE_0_K="50"
SCALE_0_RRR_SETS="100000"
SCALE_0_COMMERCIAL_FILE="graphs/scale0/commercial.csv"
SCALE_0_CYBER_FILE="graphs/scale0/cyber.csv"
SCALE_0_SOCIAL_FILE="graphs/scale0/social.csv"
SCALE_0_USES_FILE="graphs/scale0/uses.csv"
SCALE_0_LOGFILE="data/scale0_run.txt"

SCALE_1_K="100"
SCALE_1_RRR_SETS="2000000"
SCALE_1_COMMERCIAL_FILE="data/scale1/commercial.csv"
SCALE_1_CYBER_FILE="data/scale1/cyber.csv"
SCALE_1_SOCIAL_FILE="data/scale1/social.csv"
SCALE_1_USES_FILE="data/scale1/uses.csv"
SCALE_1_LOGFILE="data/scale1_run.txt"

# scale -1
export K="$SCALE_n1_K"
export RRR_SETS="$SCALE_n1_RRR_SETS"
export COMMERCIAL_FILE="$SCALE_n1_COMMERCIAL_FILE"
export CYBER_FILE="$SCALE_n1_CYBER_FILE"
export SOCIAL_FILE="$SCALE_n1_SOCIAL_FILE"
export USES_FILE="$SCALE_n1_USES_FILE"
export LOGFILE="$SCALE_n1_LOGFILE"
bash "${RUN_SCRIPT}"

# scale 0
export K="$SCALE_0_K"
export RRR_SETS="$SCALE_0_RRR_SETS"
export COMMERCIAL_FILE="$SCALE_0_COMMERCIAL_FILE"
export CYBER_FILE="$SCALE_0_CYBER_FILE"
export SOCIAL_FILE="$SCALE_0_SOCIAL_FILE"
export USES_FILE="$SCALE_0_USES_FILE"
export LOGFILE="$SCALE_0_LOGFILE"
#bash "${RUN_SCRIPT}"

# scale 1
export K="$SCALE_1_K"
export RRR_SETS="$SCALE_1_RRR_SETS"
export COMMERCIAL_FILE="$SCALE_1_COMMERCIAL_FILE"
export CYBER_FILE="$SCALE_1_CYBER_FILE"
export SOCIAL_FILE="$SCALE_1_SOCIAL_FILE"
export USES_FILE="$SCALE_1_USES_FILE"
export LOGFILE="$SCALE_1_LOGFILE"
#bash "${RUN_SCRIPT}"
