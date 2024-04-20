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
    11: "graphs/rmat_571919_seed1_scale11_nV2048_nE22601.el"
}

chunk_modes = ["no_chunk", "chunk_vert", "chunk_edge"]

def compile():
    print("Compiling ... ", end=" ")
    subprocess.call(["make", "setup"])
    subprocess.call(["make", "-C", "dockerbuild", "-j8"])
    print("DONE")

def clean():
    print("Cleaning ... ", end=" ")
    subprocess.call(["rm", "-rf", "dockerbuild"])
    print("DONE")

def run_cmd(command_line, env={}):
    args = shlex.split(command_line)
    print(args)
    proc = subprocess.Popen(args, stdout=subprocess.PIPE)
    try:
        outs = subprocess.check_output(args, env=env, stderr=subprocess.STDOUT)
        return outs.decode().split("\n")
    except subprocess.CalledProcessError as error:
        errorMessage = (
            ">>> Error while executing:\n"
            + command_line
            + "\n>>> Returned with error:\n"
            + str(error.output.decode())
        )
        with open("err.txt", "w+") as f:
            print("WRITING ERR TO FILE ... ")
            f.write(errorMessage)
        assert False
    except subprocess.TimeoutExpired:
        print("\t TIMEOUT")
        proc.kill()
        assert False
    return None

def collect_trace_DLCSR(scale=5, chunk_mode=0, num_hosts=2, num_cores=32, run=False, graph_type = 0):
    assert PXNS_TIMES_CORES % num_hosts == 0, f"PXNS_TIMES_CORES = {PXNS_TIMES_CORES} should b divisible by num_hosts = {num_hosts}"
    assert PXNS_TIMES_PE % num_hosts == 0, f"num_hosts = {num_hosts} should be even"
    pe = PXNS_TIMES_PE // num_hosts
    num_cores = PXNS_TIMES_CORES // num_hosts

    # 64 cores / numhosts ... Cpus/host
    # PE = num_hosts * cpus ...  ... min 4 cpus (32 or 64)
    # num_cores * num_hosts = localhost:32 (localhost:64 on languedoc)
    # pe = localhost:32 / num_hosts

    # Create place for trace files
    fn = f"data/DLCSR/host{num_hosts}/{chunk_modes[chunk_mode]}/scale{scale}"
    print(f"\t *** Creating {fn}")
    if not os.path.exists(fn):
        os.makedirs(fn)

    assert os.path.exists(fn), f"OS should now see a path to {fn}"

    # Generate trace files
    cmd0 = f'MPIRUN_CMD="mpirun -np %N -host localhost:32 --mca btl self,vader,tcp --map-by slot:PE={pe} --use-hwthread-cpus %P %A" PANDO_PREP_MAIN_NODE=17179869184 '
    cmd0 += f"./scripts/preprun_mpi.sh -c {num_cores} -n {num_hosts} {executable} -i {rmat_files[scale]} -v {2**scale} -c {chunk_mode} -g {graph_type}"
    # Move trace files into dir
    cmd1 = f"mv *.trace {fn}"
    if run:
        run_cmd(cmd0, env=env)
        run_cmd(cmd1)

    return [cmd0, cmd1]



def bench_DLCSR(num_hosts=8):
    print("*******************************************************")
    print(f"          Collecting Benchmarks for DLCSR ...")
    print("*******************************************************")
    all_cmds = []
    for scale in rmat_files:
        msg = f'"*** scale = {scale}, fn = {rmat_files[scale]} ***"'
        all_cmds.append(f"echo {msg}")
        for c in range(len(chunk_modes)):
            cmds = collect_trace_DLCSR(scale, c, num_hosts)
            all_cmds += cmds

    with open("bench_tc.sh", "w") as file:
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
    default=8,
)
# parser.add_argument(
#     "-c",
#     "--numCores",
#     action="store",
#     help=f"Enter number of cores per host",
#     type=int,
#     required=False,
#     default=8,
# )

args = vars(parser.parse_args())
num_hosts = int(args["numHosts"])
# num_cores = int(args["numCores"])
print("*" * 42)
print("*" + " " * 10 + f"Benching TC on {num_hosts} hosts")
print("*" * 42)

assert num_hosts > 0, f"ERROR: Num hosts = {num_hosts} should be > 0"
# assert num_cores > 0, f"ERROR: Num hosts = {num_cores} should be > 0"
bench_DLCSR(num_hosts=num_hosts)
