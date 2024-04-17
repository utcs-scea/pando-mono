## Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.

##userid generation
import sys
import sst
import os
import random

securityEnabled = 0
decryptLat = 11
aesOption = 3 # 1: encrypt, 2: decrypt, 3: both
acmCheckLat = 2
bfsSize = "10"
isACM_row_hit = 1
memOpThresh0 = 0
memOpThresh1 = 0

max_users = 2
num_applications = 2

uid_array = [i for i in range(0, max_users)]

cores = 1
memory_capacity = 4096 #in MB

instr_count = 0

niter = 1000
netBW = "80GiB/s"
debug = 2
debug_level = 0

clock = "2GHz"
memory_clock = "2GHz"
coherence_protocol = "MESI"

os.environ['OMP_NUM_THREADS'] = str(cores)

l3cache_blocks = 4
l3cache_block_size = "1KiB"

ring_latency = "50ps" # 2.66 GHz time period plus slack for ringstop latency
ring_bandwidth = "96GiB/s" # 2.66GHz clock, moves 64-bytes per cycle, plus overhead = 36B/c
ring_flit_size = "8B"

mem_interleave_size = 64    # Do 64B cache-line level interleaving

page_size = 4                   # In KB
num_pages = memory_capacity * 1024 / page_size

#app = "/proj/research_intr/users/kpunniya/pando-graph500/build/src/graph500_reference_bfs"
#app = "$HOME/pando-graph500/build/src/graph500_reference_bfs"
app = "/root/pando-graph500/build/src/graph500_reference_bfs"
app_args = 1
app_arguments = [bfsSize]

next_core_id=0
shared_memory=1
network_address = 0

ariel_params = {
    "verbose" : 1,
    "clock" : clock,
    "maxcorequeue" : 1024,
    "maxissuepercycle" : 2,
    "maxtranscore": 32,
    "pipetimeout" : 0,
    "corecount" : cores,
    "printStats" : 1,
    "memmgr.memorylevels" : 1,
    "memmgr.pagecount0" : num_pages,
    "memmgr.pagesize0" : page_size * 1024,
    "memmgr.defaultlevel" : 0,
    "arielmode" : 0,
    "launchparamcount" : 1,
    "launchparam0" : "-ifeellucky",
    "max_insts" : instr_count,
    "aes_enable_security" : securityEnabled,
}

l1_params = {
    "cache_frequency": clock,
    "cache_size": "32KiB",
    "associativity": 8,
    "access_latency_cycles": 4,
    "L1": 1,
    "verbose": 30,
	"debug_level": 9,
	"debug": 1,
    "maxRequestDelay" : "1000000000000",
}

comp_cpu = [0]*num_applications

comp_l1cache = [0]*num_applications

link_cpu_cache_link = [0]*num_applications

link_l1_ring = [0]*num_applications

mmu_params = {
        "os_page_size": 4,
	    "perfect": 0,
        "corecount": cores,
	    "sizes_L1": 4,
	    "page_size1_L1": 4,
        "page_size2_L1": 2048,
        "page_size3_L1": 1024*1024,
        "page_size4_L1": 512*512*512*4,
        "assoc1_L1": 4,
        "size1_L1": 32,
        "assoc2_L1": 4,
        "size2_L1": 32,
        "assoc3_L1": 4,
        "size3_L1": 16,
        "assoc4_L1": 4,
        "size4_L1": 4,
        "sizes_L2": 4,
        "page_size1_L2": 4,
        "page_size2_L2": 2048,
        "page_size3_L2": 1024*1024,
        "page_size4_L2": 512*512*512*4,
        "assoc1_L2": 12,
        "size1_L2": 1536,#1536,
        "assoc2_L2": 32, #12,
        "size2_L2": 32, #1536,
        "assoc3_L2": 4,
        "size3_L2": 16,
        "assoc4_L2": 4,
        "size4_L2": 4,
        "clock": clock,
        "levels": 2,
        "max_width_L1": 3,
        "max_outstanding_L1": 2,
	    "max_outstanding_PTWC": 2,
        "latency_L1": 4,
        "parallel_mode_L1": 1,
        "max_outstanding_L2": 2,
        "max_width_L2": 4,
        "latency_L2": 10,
        "parallel_mode_L2": 0,
	    "self_connected" : 0,
	    "page_walk_latency": 200,
	    "size1_PTWC": 32, # this just indicates the number entries of the page table walk cache level 1 (PTEs)
	    "assoc1_PTWC": 4, # this just indicates the associtativit the page table walk cache level 1 (PTEs)
	    "size2_PTWC": 32, # this just indicates the number entries of the page table walk cache level 2 (PMDs)
	    "assoc2_PTWC": 4, # this just indicates the associtativit the page table walk cache level 2 (PMDs)
	    "size3_PTWC": 32, # this just indicates the number entries of the page table walk cache level 3 (PUDs)
	    "assoc3_PTWC": 4, # this just indicates the associtativit the page table walk cache level 3 (PUDs)
	    "size4_PTWC": 32, # this just indicates the number entries of the page table walk cache level 4 (PGD)
	    "assoc4_PTWC": 4, # this just indicates the associtativit the page table walk cache level 4 (PGD)
	    "latency_PTWC": 10, # This is the latency of checking the page table walk cache
	    "emulate_faults": 0,
}

mmu = [0]*num_applications

MMUCacheLink = [0]*num_applications

arielMMULink = [0]*num_applications

# internal_network = Network("internal_network","0","1ns","1ns",0)

membus = sst.Component("membus", "memHierarchy.Bus")
membus.addParams( { "bus_frequency" : clock } )

for i in range(0, num_applications):
    uid = (i+1)/max_users;
    comp_cpu[i] = sst.Component("cpu"+str(i), "ariel.ariel")
    comp_cpu[i].addParams(ariel_params)
    comp_cpu[i].addParams({"executable": app})
    comp_cpu[i].addParam("appargcount", app_args)
    comp_cpu[i].addParams({"userID" : uid})
    if (i == 0):
        comp_cpu[i].addParams({"memop_violation" : memOpThresh0})
    else:
        comp_cpu[i].addParams({"memop_violation" : memOpThresh1})
    for j in range(0, app_args):
        comp_cpu[i].addParam("apparg" + str(j),app_arguments[j])

# Enable statistics outputs
    comp_cpu[i].enableAllStatistics({"type":"sst.AccumulatorStatistic"})

    mmu[i] = sst.Component("mmu"+str(i), "Samba")
    mmu[i].addParams(mmu_params)

    comp_l1cache[i] = sst.Component("l1cache"+str(i), "memHierarchy.Cache")
    comp_l1cache[i].addParams({
	    "shared_memory": shared_memory,
        "local_memory_size" : memory_capacity*1024*1024,
        "network_address" : network_address,
    })
    comp_l1cache[i].addParams(l1_params)

    # network_address = network_address+1

# Enable statistics outputs
    comp_l1cache[i].enableAllStatistics({"type":"sst.AccumulatorStatistic"})

    arielMMULink[i] = sst.Link("cpu_mmu_link_" + str(i))
    arielMMULink[i].connect((comp_cpu[i], "cache_link_%d"%next_core_id, ring_latency), (mmu[i], "cpu_to_mmu%d"%next_core_id, ring_latency))

    MMUCacheLink[i]= sst.Link("mmu_cache_link_" + str(i))
    MMUCacheLink[i].connect((mmu[i], "mmu_to_cache%d"%next_core_id, ring_latency), (comp_l1cache[i], "high_network_0", ring_latency))

    link_l1_ring[i] = sst.Link("link_l1_ring"+str(i))
    link_l1_ring[i].connect((comp_l1cache[i], "low_network_0", "50ps"), (membus, "high_network_" + str(i), "50ps"))


local_memory_params = {
    "backing" : "none",
    "backend" : "memHierarchy.timingDRAM",
    "backend.id" : 0,
    "backend.addrMapper" : "memHierarchy.roundRobinAddrMapper",
    "backend.addrMapper.interleave_size" : "64B",
    "backend.addrMapper.row_size" : "4KiB",
    "backend.clock" : "1GHz", #memory_clock,
    "backend.mem_size" : "256GiB",
    "backend.channels" : 2,
    "backend.channel.numRanks" : 2,
    "backend.channel.rank.numBanks" : 16,
    "backend.channel.transaction_Q_size" : 32,
    "backend.channel.rank.bank.CL" : 14,
    "backend.channel.rank.bank.CL_WR" : 12,
    "backend.channel.rank.bank.RCD" : 14,
    "backend.channel.rank.bank.TRP" : 14,
    "backend.channel.rank.bank.dataCycles" : 2,
    "backend.channel.rank.bank.pagePolicy" : "memHierarchy.simplePagePolicy",
    "backend.channel.rank.bank.transactionQ" : "memHierarchy.fifoTransactionQ",
    "backend.channel.rank.bank.pagePolicy.close" : 1,
    "backend.tCAS" : 13,
    "backend.tRCD" : 13,
    "backend.tRP" : 13,
    "backend.cycle_time" : "1ns",
    "backend.row_size" : "4KiB",
    "backend.row_policy" : "open",
    "clock" : "1GHz",
    "debug" : 0,
    "debug_level" : 5
}



comp_memctrl = sst.Component("local_memory", "memHierarchy.MemController")
comp_memctrl.enableAllStatistics({"type":"sst.AccumulatorStatistic"})
comp_memctrl.addParams({
	"max_requests_per_cycle" : 128,
	"clock" : "1GHz",
	"shared_memory" : shared_memory,
	"node" : 0,
    "memory_size"               : memory_capacity*1024*1024,
    "aes_enable_security"                   : securityEnabled,
    "aes_decryption_latency"                : decryptLat,
    "aes_encrypt_or_decrypt"                : aesOption,     # 1: encrypt, 2: decrypt, 3: both
    "acm_check_latency"                     : acmCheckLat,
    "acm_row_hit"                           : isACM_row_hit,
    "backing" : "none",
    "verbose" : 2,
    "debug" : 2,
    "debug_level" : 10,
})

comp_memctrl.addParams(local_memory_params)

#print("Creating memory controller RAMulator subcomponent...")
#memory = comp_memctrl.setSubComponent("backend", "memHierarchy.ramulator")
#memory.addParams({
#    "mem_size" : "4096MiB",
#    #"configFile" : "/proj/research_simu/users/afreij/ramulator/configs/DDR4-config.cfg",
#    #"configFile" : "/proj/research_intr/users/kpunniya/pando-sst-repo/ramulator/configs/DDR4-config.cfg",
#    "configFile" : "/root/sst-ramulator-src/configs/hbm4-pando-config.cfg",
#    #"configFile" : "/proj/research_intr/users/kpunniya/pando-sst-repo/pando-ramulator/configs/hbm4-pando-config.cfg",
#    #"configFile" : "/proj/research_intr/users/kpunniya/pando-sst-repo/pando-ramulator/configs/DDR4-config.cfg",
#})

l2_params = {
      "cache_frequency": clock,
      "cache_size": "256KiB",
      "associativity": 8,
      "access_latency_cycles": 6,
      "L2": 1,
      "mshr_num_entries" : 16,
      "cache_line_size": 64,
      "replacement_policy": "lru",
      "network_address" : network_address,
}

network_address = network_address+1

l2cache = sst.Component("l2cache", "memHierarchy.Cache")
l2cache.addParams(l2_params)
l2cache.addParams({
	"shared_memory": shared_memory,
	})

l2_bus_link = sst.Link("l2_bus_link")
l2_bus_link.connect( (l2cache, "high_network_0", "50ps"), (membus, "low_network_0", "50ps") )

memLink = sst.Link("mem_link")
memLink.connect((comp_memctrl, "direct_link", ring_latency), (l2cache, "low_network_0", ring_latency))

sst.setStatisticLoadLevel(7)

sst.setStatisticOutput("sst.statOutputCSV")

#output_file = "results/bfs"+bfsSize+"_security" + str(securityEnabled) +"_aesOption"+str(aesOption)+"_decryptLat"+str(decryptLat)+"_acmCheckLat"+str(acmCheckLat)+ "_acmRowHit"+str(isACM_row_hit)+".csv"
output_file = "bfs_multiUser.log"

sst.setStatisticOutputOptions( {
    "filepath"  : output_file,
    "separator" : ", "
} )
