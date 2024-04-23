# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import shlex, subprocess
import argparse
import os

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
    # 11: "graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el",
    # 12: "graphs/rmat_571919_seed1_scale12_nV4096_nE48335.el",
    # 13: "graphs/rmat_571919_seed1_scale13_nV8192_nE102016.el",
    # 14: "graphs/rmat_571919_seed1_scale14_nV16384_nE213350.el"
}

chunk_modes = ["no_chunk", "chunk_vert", "chunk_edge"]
graph_types = ["DLCSR", "MDLCSR", "DACSR"]
machines = {'languedoc':64, 'zerberus':32, 'tacc':128}

def collect_traces_RMAT(num_hosts, graph_mode, chunk_mode, scale, machine, output_dir='data'):
    compute_units = machines[machine]
    assert compute_units % num_hosts == 0, f"compute_units = {compute_units} should b divisible by num_hosts = {num_hosts}"
    pe = compute_units // num_hosts
    num_cores = compute_units // num_hosts

    # Create place for trace files
    input_graph = rmat_files[scale]
    num_input_vertices = 2**scale
    fn = f"{output_dir}/{graph_types[graph_mode]}/host{num_hosts}/{chunk_modes[chunk_mode]}/scale{scale}"
    print(f"\t *** Creating {fn}")
    if not os.path.exists(fn):
        os.makedirs(fn)

    assert os.path.exists(fn), f"OS should now see a path to {fn}"

    # Generate trace files
    cmd0 = f'MPIRUN_CMD="mpirun -np %N -host localhost:{compute_units} --mca btl self,vader,tcp --map-by slot:PE={pe} --use-hwthread-cpus %P %A" PANDO_PREP_MAIN_NODE=17179869184 '
    cmd0 += f"./scripts/preprun_mpi.sh -c {num_cores} -n {num_hosts} {executable} -i {input_graph} -v {num_input_vertices} -c {chunk_mode} -g {graph_mode}"
    # Move trace files into dir
    cmd1 = f"mv *.trace {fn}"

    return [cmd0, cmd1]


def create_slurm(num_hosts, graph_mode, chunk_mode, scale, machine, output_dir='data', time='01:00:00'):
    input_graph = rmat_files[scale]
    num_input_vertices = 2**scale
    fn = f"{output_dir}/{graph_types[graph_mode]}/host{num_hosts}/{chunk_modes[chunk_mode]}/scale{scale}"
    with open('job2run.slurm', 'w') as file:
        sbatch_settings = f'#!/bin/bash\n#SBATCH -J {fn}\n#SBATCH -o {fn}.out\n#SBATCH -e {fn}.err\n#SBATCH -t {time}\n#SBATCH -N 1\n#SBATCH -n 1\n#SBATCH -c 128\n#SBATCH --exclusive\n#SBATCH --mail-type=none\n#SBATCH --mail-user=prachi.ingle@utexas.edu\n#SBATCH -p normal\n'
        file.write(f"{sbatch_settings}\n")

        echos = f'''echo "SLURM_JOB_ID=\$SLURM_JOB_ID"\necho "TIME={time}"\n'''
        file.write(f"{echos}\n")

        file.write(". ./scripts/tacc_env.sh")

        print(f"\t *** Creating {fn}")
        if not os.path.exists(fn):
            os.makedirs(fn)






def experiment_chunk_DLCSR_RMAT(num_hosts, machine, output_dir='data'):
    print("*******************************************************")
    print(f"          Collecting Benchmarks for CHUNKING on DLCSR ...")
    print("*******************************************************")
    all_cmds = []
    for scale in rmat_files:
        msg = f'"*** scale = {scale}, fn = {rmat_files[scale]} ***"'
        all_cmds.append(f"echo {msg}")
        for c in range(len(chunk_modes)):
            cmds = collect_traces_RMAT(num_hosts, 0, c, scale, machine, output_dir=f'{output_dir}/expChunk')
            all_cmds += cmds

    with open(f"experiment_chunk_{machine}_hosts{num_hosts}.sh", "w") as file:
        for line in all_cmds:
            file.write(f"{line}\n")

def experiment_graph_type_RMAT(num_hosts, machine, output_dir='data'):
    print("*******************************************************")
    print(f"          Collecting Benchmarks for GRAPH TYPES ...")
    print("*******************************************************")
    all_cmds = []
    for scale in rmat_files:
        msg = f'"*** scale = {scale}, fn = {rmat_files[scale]} ***"'
        all_cmds.append(f"echo {msg}")
        for g in range(len(graph_types)):
            input_graph = rmat_files[scale]
            cmds = collect_traces_RMAT(num_hosts, g, 0, scale, machine, output_dir=f'{output_dir}/expGT')
            all_cmds += cmds

    with open(f"experiment_graph_type_{machine}_hosts{num_hosts}.sh", "w") as file:
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
# parser.add_argument(
#     "-c",
#     "--numCores",
#     action="store",
#     help=f"Enter number of cores",
#     type=int,
#     required=True,
# )
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
parser.add_argument(
    "-m",
    "--machine",
    help=f"Specify machine: Options = {machines.keys()}",
    choices=machines,
    action='store',
    type=str,
    required=True,
    default='zerberus'
)

args = vars(parser.parse_args())
num_hosts = int(args["numHosts"])
output_dir = str(args['outputDir'])
machine = str(args['machine'])

assert num_hosts > 0, f"ERROR: Num hosts = {num_hosts} should be > 0"
assert machine in machines, f'ERROR: Dont recognize machine: {machine}'

experiment = experiment_chunk_DLCSR_RMAT if bool(args['experiment']) else experiment_graph_type_RMAT
compute_units = machines[machine]

print("*" * 42)
print("*" + " " * 10 + f"Generating bench scripts for {machine} (compute_units = {compute_units}): TC on num_hosts: {num_hosts}, num_cores: {compute_units//num_hosts}, outputDir: {output_dir}")
print("*" * 42)


experiment(num_hosts=num_hosts, output_dir=output_dir, machine=machine)
