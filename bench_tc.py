# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import shlex, subprocess
import argparse
import os

PXNS_TIMES_CORES = 64
PXNS_TIMES_PE = 32 # zerberus ... on lanuedoc, 64

# Generates bash script to run and collect benchmarks
env = {"PANDO_PREP_MAIN_NODE": "17179869184"}
executable = "./dockerbuild/microbench/triangle-counting/src/tc"

rmat_files = {
    5: "graphs/rmat_571919_seed1_scale5_nV32_nE153.el",
    6: "graphs/rmat_571919_seed1_scale6_nV64_nE403.el",
    7: "graphs/rmat_571919_seed1_scale7_nV128_nE942.el",
    8: "graphs/rmat_571919_seed1_scale8_nV256_nE2144.el",
    9: "graphs/rmat_571919_seed1_scale9_nV512_nE4736.el",
    10: "graphs/rmat_571919_seed1_scale10_nV1024_nE10447.el",
    11: "graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el",
    12: "graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el",
    13: "graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el",
    14: "graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el"
}

chunk_modes = ["no_chunk", "chunk_vert", "chunk_edge"]
graph_types = ["DLCSR", "MDLCSR", "DACSR"]

def collect_traces_RMAT(num_hosts, num_cores, graph_mode, chunk_mode, scale, output_dir='data'):
    assert PXNS_TIMES_CORES % num_hosts == 0, f"PXNS_TIMES_CORES = {PXNS_TIMES_CORES} should b divisible by num_hosts = {num_hosts}"
    assert PXNS_TIMES_PE % num_hosts == 0, f"num_hosts = {num_hosts} should be even"
    pe = PXNS_TIMES_PE // num_hosts
    num_cores = PXNS_TIMES_CORES // num_hosts

    # 64 cores / numhosts ... Cpus/host
    # PE = num_hosts * cpus ...  ... min 4 cpus (32 or 64)
    # num_cores * num_hosts = localhost:32 (localhost:64 on languedoc)
    # pe = localhost:32 / num_hosts

    # Create place for trace files
    input_graph = rmat_files[scale]
    num_input_vertices = 2**scale
    fn = f"{output_dir}/{graph_types[graph_mode]}/host{num_hosts}/{chunk_modes[chunk_mode]}/scale{scale}"
    print(f"\t *** Creating {fn}")
    if not os.path.exists(fn):
        os.makedirs(fn)

    assert os.path.exists(fn), f"OS should now see a path to {fn}"

    # Generate trace files
    cmd0 = f'MPIRUN_CMD="mpirun -np %N -host localhost:{PXNS_TIMES_PE} --mca btl self,vader,tcp --map-by slot:PE={pe} --use-hwthread-cpus %P %A" PANDO_PREP_MAIN_NODE=17179869184 '
    cmd0 += f"./scripts/preprun_mpi.sh -c {num_cores} -n {num_hosts} {executable} -i {input_graph} -v {num_input_vertices} -c {chunk_mode} -g {graph_mode}"
    # Move trace files into dir
    cmd1 = f"mv *.trace {fn}"

    return [cmd0, cmd1]


def experiment_chunk_DLCSR_RMAT(num_hosts, num_cores, output_dir='data'):
    print("*******************************************************")
    print(f"          Collecting Benchmarks for CHUNKING on DLCSR ...")
    print("*******************************************************")
    all_cmds = []
    for scale in rmat_files:
        msg = f'"*** scale = {scale}, fn = {rmat_files[scale]} ***"'
        all_cmds.append(f"echo {msg}")
        for c in range(len(chunk_modes)):
            cmds = collect_traces_RMAT(num_hosts, num_cores, 0, c, scale, output_dir=f'{output_dir}/expChunk')
            all_cmds += cmds

    with open("experiment_chunk.sh", "w") as file:
        for line in all_cmds:
            file.write(f"{line}\n")

def experiment_graph_type_RMAT(num_hosts, num_cores, output_dir='data'):
    print("*******************************************************")
    print(f"          Collecting Benchmarks for GRAPH TYPES ...")
    print("*******************************************************")
    all_cmds = []
    for scale in rmat_files:
        msg = f'"*** scale = {scale}, fn = {rmat_files[scale]} ***"'
        all_cmds.append(f"echo {msg}")
        for g in range(len(graph_types)):
            input_graph = rmat_files[scale]
            cmds = collect_traces_RMAT(num_hosts, num_cores, g, 0, scale, output_dir=f'{output_dir}/expGT')
            all_cmds += cmds

    with open("experiment_graph_type.sh", "w") as file:
        for line in all_cmds:
            file.write(f"{line}\n")




# ------------------- RUN ---------------------
parser = argparse.ArgumentParser(prog="BenchMarkTC", description="Benchmarks TC on PREP")
parser.add_argument(
    "-n",
    "--numHosts",
    action="store",
    help=f"Enter number of hosts",
    type=int,
    required=True,
)
parser.add_argument(
    "-c",
    "--numCores",
    action="store",
    help=f"Enter number of cores",
    type=int,
    required=True,
)
parser.add_argument(
    "-e",
    "--experiment",
    help=f"Choose experiment: True: experiment_chunk_DLCSR_RMAT, False: experiment_graph_type_RMAT",
    action='store_true',
    required=False,
    default=False
)
parser.add_argument(
    "-o",
    "--outputDir",
    help=f"Choose outputDir",
    action="store",
    type=str,
    required=True
)

args = vars(parser.parse_args())
num_hosts = int(args["numHosts"])
num_cores = int(args["numCores"])
output_dir = str(args['outputDir'])
experiment = experiment_chunk_DLCSR_RMAT if bool(args['experiment']) else experiment_graph_type_RMAT
print("*" * 42)
print("*" + " " * 10 + f"Generating bench scripts: TC on num_hosts: {num_hosts}, num_cores: {num_cores}, outputDir: {output_dir}")
print("*" * 42)

assert num_hosts > 0, f"ERROR: Num hosts = {num_hosts} should be > 0"
assert num_cores > 0, f"ERROR: Num hosts = {num_cores} should be > 0"
experiment(num_hosts=num_hosts, num_cores=num_cores, output_dir=output_dir)
