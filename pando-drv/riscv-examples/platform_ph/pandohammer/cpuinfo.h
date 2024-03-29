// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef PANDOHAMMER_CPUINFO_H
#define PANDOHAMMER_CPUINFO_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MCSR_MCOREID    0xF15
#define MCSR_MPODID     0xF16
#define MCSR_MPXNID     0xF17
#define MCSR_MCOREHARTS 0xF18
#define MCSR_MPODCORES  0xF19
#define MCSR_MPXNPODS   0xF1A
#define MCSR_MNUMPXN    0xF1B
#define MCSR_MCOREL1SPSIZE 0xF1C
#define MCSR_MPODL2SPSIZE  0xF1D
#define MCSR_MPXNDRAMSIZE  0xF1E

#ifndef __stringify
#define __stringify_1(x) #x
#define __stringify(x) __stringify_1(x)
#endif

/**
 * thread id wrt my core
 */
inline int myThreadId()
{
    int64_t tid;
    asm volatile ("csrr %0, mhartid" : "=r"(tid));
    return (int)tid;
}

/**
 * core id wrt my pod
 */
inline int myCoreId()
{
    int64_t cid;
    asm volatile ("csrr %0, " __stringify(MCSR_MCOREID) : "=r"(cid));
    return (int)cid;
}

/**
 * pod id wrt my pxn
 */
inline int myPodId()
{
    int64_t pid;
    asm volatile ("csrr %0, " __stringify(MCSR_MPODID) : "=r"(pid));
    return (int)pid;
}

/**
 * pxn id
 */
inline int myPXNId()
{
    int64_t xid;
    asm volatile ("csrr %0, " __stringify(MCSR_MPXNID) : "=r"(xid));
    return (int)xid;
}

/**
 * number of hardware threads on my core
 */
inline int myCoreThreads()
{
    int64_t harts;
    asm volatile ("csrr %0, " __stringify(MCSR_MCOREHARTS) : "=r"(harts));
    return (int)harts;
}

/**
 * number of pxns in system
 */
inline int numPXN()
{
    int64_t num;
    asm volatile ("csrr %0, " __stringify(MCSR_MNUMPXN) : "=r"(num));
    return (int)num;
}

/**
 * number of cores in a pod
 */
inline int numPodCores()
{
    int64_t cores;
    asm volatile ("csrr %0, " __stringify(MCSR_MPODCORES) : "=r"(cores));
    return (int)cores;
}

/**
 * number of pods in a pxn
 */
inline int numPXNPods()
{
    int64_t pods;
    asm volatile ("csrr %0, " __stringify(MCSR_MPXNPODS) : "=r"(pods));
    return (int)pods;
}

/**
 * size of l1sp in bytes
 */
inline uint64_t coreL1SPSize() {
    uint64_t l1sp_size;
    asm volatile ("csrr %0, " __stringify(MCSR_MCOREL1SPSIZE) : "=r"(l1sp_size));
    return l1sp_size;
}

/**
 * size of l2sp in bytes
 */
inline uint64_t podL2SPSize() {
    uint64_t l2sp_size;
    asm volatile ("csrr %0, " __stringify(MCSR_MPODL2SPSIZE) : "=r"(l2sp_size));
    return l2sp_size;
}

/**
 * size of pxn's dram in bytes
 */
inline uint64_t pxnDRAMSize() {
    uint64_t dram_size;
    asm volatile ("csrr %0, " __stringify(MCSR_MPXNDRAMSIZE) : "=r"(dram_size));
    return dram_size;
}

/**
 * get the current cycle count
 */
inline uint64_t cycle() {
    uint64_t cycle;
    asm volatile ("rdcycle %0" : "=r"(cycle));
    return cycle;
}

#ifdef __cplusplus
}
#endif
#endif
