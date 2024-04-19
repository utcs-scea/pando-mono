# SPDX-License-Identifier: MIT
# Copyright (c) 2023. University of Texas at Austin. All rights reserved.

import sys
import os
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) != 8:
    print("usage: python3 trace_process.py (application or workflow name) (number of hosts) (directory to the stat files) (per-phase / end-to-end) (include-local / remote-only) (load / store / rmw / func / reference) (stat-dump / plot-hist)")
    sys.exit(1)

app = sys.argv[1]
host = int(sys.argv[2])
directory = sys.argv[3]
granularity = sys.argv[4]
print_local = sys.argv[5]
access_type = sys.argv[6]
result_mode = sys.argv[7]

#####   Preprocess trace files   #####
def extract(filename, app):
    with open(filename, 'r') as file:
        capturing = False
        lines_to_keep = []
        for line in file:
            if app in line:
                capturing = not capturing
            if capturing or app in line:
                lines_to_keep.append(line)

    with open(filename, 'w') as file:
        file.writelines(lines_to_keep)

def process_files(file_list, app):
    for filename in file_list:
        extract(filename, app)

file_list = []
for host_id in range(host):
    file_list.append(directory + '/pando_mem_stat_node_' + str(host_id) + '.trace')
process_files(file_list, app)

os.makedirs("trace", exist_ok=True)
for file in file_list:
    os.system("mv " + file + " ./trace")



#####   Processing   #####
load_cnt = []
store_cnt = []
rmw_cnt = []
func_cnt = []
waitgroup_cnt = []
load_byte = []
store_byte = []
rmw_byte = []
func_byte = []
waitgroup_byte = []

phase_total = [0 for i in range(host)]

for host_id in range(host):
    load_cnt.append([])
    store_cnt.append([])
    rmw_cnt.append([])
    func_cnt.append([])
    waitgroup_cnt.append([])
    load_byte.append([])
    store_byte.append([])
    rmw_byte.append([])
    func_byte.append([])
    waitgroup_byte.append([])

    phase_curr = -1

    src = -1

    print("Reading host " + str(host_id))

    filename = "./trace/pando_mem_stat_node_" + str(host_id) + ".trace"
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
            waitgroup_cnt[host_id].append([0 for src in range(host)])
            load_byte[host_id].append([0 for src in range(host)])
            store_byte[host_id].append([0 for src in range(host)])
            rmw_byte[host_id].append([0 for src in range(host)])
            func_byte[host_id].append([0 for src in range(host)])
            waitgroup_byte[host_id].append([0 for src in range(host)])
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
        elif line.find('WAIT_GROUP (count)') != -1:
            line_split = line.strip().split()
            count = int(line_split[2])
            waitgroup_cnt[host_id][phase_curr][src] += count
        elif line.find('WAIT_GROUP (bytes)') != -1:
            line_split = line.strip().split()
            byte = int(line_split[2])
            waitgroup_byte[host_id][phase_curr][src] += byte



# adjust the load numbers
for host_id in range(host):
    for phase in range(phase_total[host_id]):
        for src in range(host):
            if waitgroup_cnt[host_id][phase][src] > load_cnt[host_id][phase][src]:
                print("difference = ", waitgroup_cnt[host_id][phase][src] - load_cnt[host_id][phase][src])
            if waitgroup_cnt[host_id][phase][src] > 0:
                load_cnt[host_id][phase][src] = load_cnt[host_id][phase][src] - waitgroup_cnt[host_id][phase][src] + 1



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
    if print_local == "remote-only":
        temp[host_id] = 0
    ax[0].bar(x, temp)
    temp = byte_array[host_id][phase]
    if print_local == "remote-only":
        temp[host_id] = 0
    ax[1].bar(x, temp)
    ax[0].set_xticks(x)
    ax[1].set_xticks(x)
    ax[0].set_xlabel("Host ID")
    ax[1].set_xlabel("Host ID")
    ax[0].set_ylabel("Number of " + access_type + "s")
    ax[1].set_ylabel("Bytes")
    if send_recv == "send":
        fig.suptitle(access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")")
    elif send_recv == "recv":
        fig.suptitle(access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")")
    fig.savefig("./histograms/" + access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_phase" + str(phase) + ".png", dpi=fig.dpi)
    plt.close(fig)

def plot_total(access_type, send_recv, host_id, x, cnt_array, byte_array):
    fig, ax = plt.subplots(nrows=1, ncols=2, figsize = (10, 5))
    temp = cnt_array[host_id]
    if print_local == "remote-only":
        temp[host_id] = 0
    ax[0].bar(x, temp)
    temp = byte_array[host_id]
    if print_local == "remote-only":
        temp[host_id] = 0
    ax[1].bar(x, temp)
    ax[0].set_xticks(x)
    ax[1].set_xticks(x)
    ax[0].set_xlabel("Host ID")
    ax[1].set_xlabel("Host ID")
    ax[0].set_ylabel("Number of " + access_type + "s")
    ax[1].set_ylabel("Bytes")
    if send_recv == "send":
        fig.suptitle(access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Total)")
    elif send_recv == "recv":
        fig.suptitle(access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Total)")
    fig.savefig("./histograms/" + access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_total.png", dpi=fig.dpi)
    plt.close(fig)

def dump_phase(access_type, send_recv, host_id, phase, x, cnt_array, byte_array):
    file = open("./stat/" + access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_phase" + str(phase) + ".txt", "w")
    if send_recv == "send":
        file.write(access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")\n")
        prep = "to"
    elif send_recv == "recv":
        file.write(access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Phase " + str(phase) + ")\n")
        prep = "from"
    file.write("\n")
    for i in range(host):
        file.write(prep + " host " + str(i) + " : " + str(cnt_array[host_id][phase][i]) + " requests, " + str(byte_array[host_id][phase][i]) + " bytes\n")
    file.close()

def dump_total(access_type, send_recv, host_id, x, cnt_array, byte_array):
    file = open("./stat/" + access_type.lower() + "_" + send_recv + "_host" + str(host_id) + "_total.txt", "w")
    if send_recv == "send":
        file.write(access_type + "s Sent By Host " + str(host_id) + " (" + app.upper() + " Total)\n")
        prep = "to"
    elif send_recv == "recv":
        file.write(access_type + "s Received By Host " + str(host_id) + " (" + app.upper() + " Total)\n")
        prep = "from"
    file.write("\n")
    for i in range(host):
        file.write(prep + " host " + str(i) + " : " + str(cnt_array[host_id][i]) + " requests, " + str(byte_array[host_id][i]) + " bytes\n")
    file.close()

if result_mode == "stat-dump":
    os.makedirs("stat", exist_ok=True)
elif result_mode == "plot-hist":
    os.makedirs("histograms", exist_ok=True)

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
            if result_mode == "stat-dump":
                if access_type == "load":
                    dump_phase("Load", "recv", host_id, phase, x, load_cnt, load_byte)
                    dump_phase("Load", "send", host_id, phase, x, load_cnt_trans, load_byte_trans)
                elif access_type == "store":
                    dump_phase("Store", "recv", host_id, phase, x, store_cnt, store_byte)
                    dump_phase("Store", "send", host_id, phase, x, store_cnt_trans, store_byte_trans)
                elif access_type == "rmw":
                    dump_phase("RMW", "recv", host_id, phase, x, rmw_cnt, rmw_byte)
                    dump_phase("RMW", "send", host_id, phase, x, rmw_cnt_trans, rmw_byte_trans)
                elif access_type == "func":
                    dump_phase("Function", "recv", host_id, phase, x, func_cnt, func_byte)
                    dump_phase("Function", "send", host_id, phase, x, func_cnt_trans, func_byte_trans)
                elif access_type == "reference":
                    dump_phase("Reference", "recv", host_id, phase, x, ref_cnt, ref_byte)
                    dump_phase("Reference", "send", host_id, phase, x, ref_cnt_trans, ref_byte_trans)
            elif result_mode == "plot-hist":
                if access_type == "load":
                    plot_phase("Load", "recv", host_id, phase, x, load_cnt, load_byte)
                    plot_phase("Load", "send", host_id, phase, x, load_cnt_trans, load_byte_trans)
                elif access_type == "store":
                    plot_phase("Store", "recv", host_id, phase, x, store_cnt, store_byte)
                    plot_phase("Store", "send", host_id, phase, x, store_cnt_trans, store_byte_trans)
                elif access_type == "rmw":
                    plot_phase("RMW", "recv", host_id, phase, x, rmw_cnt, rmw_byte)
                    plot_phase("RMW", "send", host_id, phase, x, rmw_cnt_trans, rmw_byte_trans)
                elif access_type == "func":
                    plot_phase("Function", "recv", host_id, phase, x, func_cnt, func_byte)
                    plot_phase("Function", "send", host_id, phase, x, func_cnt_trans, func_byte_trans)
                elif access_type == "reference":
                    plot_phase("Reference", "recv", host_id, phase, x, ref_cnt, ref_byte)
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
        if result_mode == "stat-dump":
            if access_type == "load":
                dump_total("Load", "recv", host_id, x, load_cnt_total, load_byte_total)
                dump_total("Load", "send", host_id, x, load_cnt_total_trans, load_byte_total_trans)
            elif access_type == "store":
                dump_total("Store", "recv", host_id, x, store_cnt_total, store_byte_total)
                dump_total("Store", "send", host_id, x, store_cnt_total_trans, store_byte_total_trans)
            elif access_type == "rmw":
                dump_total("RMW", "recv", host_id, x, rmw_cnt_total, rmw_byte_total)
                dump_total("RMW", "send", host_id, x, rmw_cnt_total_trans, rmw_byte_total_trans)
            elif access_type == "func":
                dump_total("Function", "recv", host_id, x, func_cnt_total, func_byte_total)
                dump_total("Function", "send", host_id, x, func_cnt_total_trans, func_byte_total_trans)
            elif access_type == "reference":
                dump_total("Reference", "recv", host_id, x, ref_cnt_total, ref_byte_total)
                dump_total("Reference", "send", host_id, x, ref_cnt_total_trans, ref_byte_total_trans)
        elif result_mode == "plot-hist":
            if access_type == "load":
                plot_total("Load", "recv", host_id, x, load_cnt_total, load_byte_total)
                plot_total("Load", "send", host_id, x, load_cnt_total_trans, load_byte_total_trans)
            elif access_type == "store":
                plot_total("Store", "recv", host_id, x, store_cnt_total, store_byte_total)
                plot_total("Store", "send", host_id, x, store_cnt_total_trans, store_byte_total_trans)
            elif access_type == "rmw":
                plot_total("RMW", "recv", host_id, x, rmw_cnt_total, rmw_byte_total)
                plot_total("RMW", "send", host_id, x, rmw_cnt_total_trans, rmw_byte_total_trans)
            elif access_type == "func":
                plot_total("Function", "recv", host_id, x, func_cnt_total, func_byte_total)
                plot_total("Function", "send", host_id, x, func_cnt_total_trans, func_byte_total_trans)
            elif access_type == "reference":
                plot_total("Reference", "recv", host_id, x, ref_cnt_total, ref_byte_total)
                plot_total("Reference", "send", host_id, x, ref_cnt_total_trans, ref_byte_total_trans)
else:
    sys.exit(3)
