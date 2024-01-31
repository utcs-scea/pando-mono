# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys
import os
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) != 5:
    print("usage: python3 plot.py (application or workflow name) (number of hosts) (directory to the stat files) (per-phase / end-to-end)")
    sys.exit(1)

app = sys.argv[1]
host = int(sys.argv[2])
directory = sys.argv[3]
granularity = sys.argv[4]

#####   Processing   #####
load_cnt = []
store_cnt = []
rmw_cnt = []
func_cnt = []
load_byte = []
store_byte = []
rmw_byte = []
func_byte = []

phase_total = [0 for i in range(host)]

for host_id in range(host):
    load_cnt.append([])
    store_cnt.append([])
    rmw_cnt.append([])
    func_cnt.append([])
    load_byte.append([])
    store_byte.append([])
    rmw_byte.append([])
    func_byte.append([])

    phase_curr = -1

    src = -1

    print("Reading host " + str(host_id))

    filename = directory + "/pando_mem_stat_node_" + str(host_id) + ".trace"
    f = open(filename, "r")
    #lines = f.readlines()

    for line in f:
        if line.find('Destination Node') != -1:
            line_split = line.strip().split()
            dst = int(line_split[2])
            if (dst != host_id):
                sys.exit(2)
        elif line.find('Phase') != -1:
            phase_curr += 1
            phase_total[host_id] += 1

            load_cnt[host_id].append([0 for src in range(host)])
            store_cnt[host_id].append([0 for src in range(host)])
            rmw_cnt[host_id].append([0 for src in range(host)])
            func_cnt[host_id].append([0 for src in range(host)])
            load_byte[host_id].append([0 for src in range(host)])
            store_byte[host_id].append([0 for src in range(host)])
            rmw_byte[host_id].append([0 for src in range(host)])
            func_byte[host_id].append([0 for src in range(host)])
        elif line.find('Source Node') != -1:
            line_split = line.strip().split()
            src = int(line_split[2])
        ### ATOMIC should check first!
        elif line.find('ATOMIC_LOAD (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            load_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_LOAD (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            load_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_STORE (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_STORE (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_COMPARE_EXCHANGE (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_COMPARE_EXCHANGE (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_INCREMENT (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_INCREMENT (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_DECREMENT (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_DECREMENT (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_FETCH_ADD (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_FETCH_ADD (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('ATOMIC_FETCH_SUB (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            rmw_cnt[host_id][phase_curr][src] += count
        elif line.find('ATOMIC_FETCH_SUB (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            rmw_byte[host_id][phase_curr][src] += byte
        elif line.find('LOAD (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            load_cnt[host_id][phase_curr][src] += count
        elif line.find('LOAD (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            load_byte[host_id][phase_curr][src] += byte
        elif line.find('STORE (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            store_cnt[host_id][phase_curr][src] += count
        elif line.find('STORE (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            store_byte[host_id][phase_curr][src] += byte
        elif line.find('FUNC (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            func_cnt[host_id][phase_curr][src] += count
        elif line.find('FUNC (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            func_byte[host_id][phase_curr][src] += byte



# adjust number of phases for each node
phase_max = max(phase_total)
for host_id in range(host):
    phase_diff = phase_max - phase_total[host_id]
    for i in range(phase_diff):
        load_cnt[host_id].append([0 for src in range(host)])
        store_cnt[host_id].append([0 for src in range(host)])
        rmw_cnt[host_id].append([0 for src in range(host)])
        func_cnt[host_id].append([0 for src in range(host)])
        load_byte[host_id].append([0 for src in range(host)])
        store_byte[host_id].append([0 for src in range(host)])
        rmw_byte[host_id].append([0 for src in range(host)])
        func_byte[host_id].append([0 for src in range(host)])



#####   Plotting   #####
def plot_phase(access_type, send_recv, host_id, phase, x, cnt_array, byte_array):
    fig, ax = plt.subplots(nrows=1, ncols=2, figsize = (10, 5))
    temp = cnt_array[host_id][phase]
    temp[host_id] = 0
    ax[0].bar(x, temp)
    temp = byte_array[host_id][phase]
    temp[host_id] = 0
    ax[1].bar(x, temp)
    ax[0].set_xticks(x)
    ax[1].set_xticks(x)
    ax[0].set_xlabel("Host ID")
    ax[1].set_xlabel("Host ID")
    ax[0].set_ylabel("Number of " + access_type + "s")
    ax[1].set_ylabel("Bytes")
    if send_recv == "send":
        fig.suptitle("Remote " + access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")")
    elif send_recv == "recv":
        fig.suptitle("Remote " + access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")")
    fig.savefig(access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_phase" + str(phase) + ".png", dpi=fig.dpi)
    plt.close(fig)

def plot_total(access_type, send_recv, host_id, x, cnt_array, byte_array):
    fig, ax = plt.subplots(nrows=1, ncols=2, figsize = (10, 5))
    temp = cnt_array[host_id]
    temp[host_id] = 0
    ax[0].bar(x, temp)
    temp = byte_array[host_id]
    temp[host_id] = 0
    ax[1].bar(x, temp)
    ax[0].set_xticks(x)
    ax[1].set_xticks(x)
    ax[0].set_xlabel("Host ID")
    ax[1].set_xlabel("Host ID")
    ax[0].set_ylabel("Number of " + access_type + "s")
    ax[1].set_ylabel("Bytes")
    if send_recv == "send":
        fig.suptitle("Remote " + access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Total)")
    elif send_recv == "recv":
        fig.suptitle("Remote " + access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Total)")
    fig.savefig(access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_total.png", dpi=fig.dpi)
    plt.close(fig)

os.makedirs("histograms", exist_ok=True)
cwd = os.getcwd()
os.chdir(cwd + "/histograms/")
if granularity == "per-phase":
    load_cnt_trans = np.transpose(load_cnt)
    store_cnt_trans = np.transpose(store_cnt)
    rmw_cnt_trans = np.transpose(rmw_cnt)
    func_cnt_trans = np.transpose(func_cnt)
    load_byte_trans = np.transpose(load_byte)
    store_byte_trans = np.transpose(store_byte)
    rmw_byte_trans = np.transpose(rmw_byte)
    func_byte_trans = np.transpose(func_byte)

    ref_cnt = np.array([[[0 for src in range(host)] for phase in range(phase_max)] for dst in range(host)])
    ref_byte = np.array([[[0 for src in range(host)] for phase in range(phase_max)] for dst in range(host)])
    for dst in range(host):
        for phase in range(phase_max):
            for src in range(host):
                ref_cnt[dst][phase][src] += (load_cnt[dst][phase][src] + store_cnt[dst][phase][src] + rmw_cnt[dst][phase][src] + func_cnt[dst][phase][src] )
                ref_byte[dst][phase][src] += (load_byte[dst][phase][src] + store_byte[dst][phase][src] + rmw_byte[dst][phase][src] + func_byte[dst][phase][src] )
    ref_cnt_trans = np.transpose(ref_cnt)
    ref_byte_trans = np.transpose(ref_byte)

    x = [i for i in range(host)]
    for host_id in range(host):
        for phase in range(phase_max):
            plot_phase("Load", "recv", host_id, phase, x, load_cnt, load_byte)
            plot_phase("Store", "recv", host_id, phase, x, store_cnt, store_byte)
            plot_phase("RMW", "recv", host_id, phase, x, rmw_cnt, rmw_byte)
            plot_phase("Function", "recv", host_id, phase, x, func_cnt, func_byte)
            plot_phase("Reference", "recv", host_id, phase, x, ref_cnt, ref_byte)

            plot_phase("Load", "send", host_id, phase, x, load_cnt_trans, load_byte_trans)
            plot_phase("Store", "send", host_id, phase, x, store_cnt_trans, store_byte_trans)
            plot_phase("RMW", "send", host_id, phase, x, rmw_cnt_trans, rmw_byte_trans)
            plot_phase("Function", "send", host_id, phase, x, func_cnt_trans, func_byte_trans)
            plot_phase("Reference", "send", host_id, phase, x, ref_cnt_trans, ref_byte_trans)

elif granularity == "end-to-end":
    load_cnt_total = np.array([[0 for src in range(host)] for dst in range(host)])
    store_cnt_total = np.array([[0 for src in range(host)] for dst in range(host)])
    rmw_cnt_total = np.array([[0 for src in range(host)] for dst in range(host)])
    func_cnt_total = np.array([[0 for src in range(host)] for dst in range(host)])
    load_byte_total = np.array([[0 for src in range(host)] for dst in range(host)])
    store_byte_total = np.array([[0 for src in range(host)] for dst in range(host)])
    rmw_byte_total = np.array([[0 for src in range(host)] for dst in range(host)])
    func_byte_total = np.array([[0 for src in range(host)] for dst in range(host)])

    for dst in range(host):
        for phase in range(phase_max):
            for src in range(host):
                load_cnt_total[dst][src] += load_cnt[dst][phase][src]
                store_cnt_total[dst][src] += store_cnt[dst][phase][src]
                rmw_cnt_total[dst][src] += rmw_cnt[dst][phase][src]
                func_cnt_total[dst][src] += func_cnt[dst][phase][src]
                load_byte_total[dst][src] += load_byte[dst][phase][src]
                store_byte_total[dst][src] += store_byte[dst][phase][src]
                rmw_byte_total[dst][src] += rmw_byte[dst][phase][src]
                func_byte_total[dst][src] += func_byte[dst][phase][src]

    load_cnt_total_trans = np.transpose(load_cnt_total)
    store_cnt_total_trans = np.transpose(store_cnt_total)
    rmw_cnt_total_trans = np.transpose(rmw_cnt_total)
    func_cnt_total_trans = np.transpose(func_cnt_total)
    load_byte_total_trans = np.transpose(load_byte_total)
    store_byte_total_trans = np.transpose(store_byte_total)
    rmw_byte_total_trans = np.transpose(rmw_byte_total)
    func_byte_total_trans = np.transpose(func_byte_total)

    ref_cnt_total = np.array([[0 for src in range(host)] for dst in range(host)])
    ref_byte_total = np.array([[0 for src in range(host)] for dst in range(host)])
    for dst in range(host):
        for src in range(host):
            ref_cnt_total[dst][src] += (load_cnt_total[dst][src] + store_cnt_total[dst][src] + rmw_cnt_total[dst][src] + func_cnt_total[dst][src])
            ref_byte_total[dst][src] += (load_byte_total[dst][src] + store_byte_total[dst][src] + rmw_byte_total[dst][src] + func_byte_total[dst][src])
    ref_cnt_total_trans = np.transpose(ref_cnt_total)
    ref_byte_total_trans = np.transpose(ref_byte_total)

    x = [i for i in range(host)]
    for host_id in range(host):
        plot_total("Load", "recv", host_id, x, load_cnt_total, load_byte_total)
        plot_total("Store", "recv", host_id, x, store_cnt_total, store_byte_total)
        plot_total("RMW", "recv", host_id, x, rmw_cnt_total, rmw_byte_total)
        plot_total("Function", "recv", host_id, x, func_cnt_total, func_byte_total)
        plot_total("Reference", "recv", host_id, x, ref_cnt_total, ref_byte_total)

        plot_total("Load", "send", host_id, x, load_cnt_total_trans, load_byte_total_trans)
        plot_total("Store", "send", host_id, x, store_cnt_total_trans, store_byte_total_trans)
        plot_total("RMW", "send", host_id, x, rmw_cnt_total_trans, rmw_byte_total_trans)
        plot_total("Function", "send", host_id, x, func_cnt_total_trans, func_byte_total_trans)
        plot_total("Reference", "send", host_id, x, ref_cnt_total_trans, ref_byte_total_trans)
else:
    sys.exit(3)
