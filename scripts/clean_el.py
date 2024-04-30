# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import argparse
import random


def fix_file(filename):
    output_file = f"{filename.replace('.el','')}_out.el"
    lines = None
    with open(filename, "r") as src:
        print(f"Opened {filename}")
        lines = [line.rstrip() for line in src]
        edges = list(sorted(list(
            set([tuple(sorted([int(_) for _ in line.split(" ")])) for line in lines])
        )))
        edges = [e for e in edges if e[0] < e[1]]
        print(f"EDGES: {edges}")

    if edges:
        with open(output_file, 'w') as dst_file:
            dst_file.writelines(
                    f"{l[0]} {l[1]}\n"
                    for i, l in enumerate(edges)
                )
        print(f"DONE Writing {output_file}\n")


parser = argparse.ArgumentParser(prog="Edge List Cleaner", description="cleans edge lists takes in fname.el and outputs fname_out.el")
parser.add_argument(
    "-i",
    "--inputEL",
    action="store",
    help=f"Enter path for EL to split",
    type=str,
    required=True,
)


args = vars(parser.parse_args())
filename = args["inputEL"]
fix_file(filename)
