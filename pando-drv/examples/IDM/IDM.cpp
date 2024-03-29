// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Michigan

#include <DrvAPIMain.hpp>
#include <DrvAPIThread.hpp>
#include <DrvAPIMemory.hpp>
#include <DrvAPI.hpp>

using namespace DrvAPI;

#include <cstdio>
#include <inttypes.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include "inputs/binaryArr.h"

// [OPTIONS]
#define WITH_IDM
#define OUTPUT

// [ENV & PARAMS]
constexpr int SIM_PXN = 8;

constexpr int CACHE_SIZE = 512; // two caches use (CACHE_SIZE) * 11 bytes L2SP
constexpr int PREFETCH_AHEAD_MIN = 2;
constexpr int PREFETCH_AHEAD_MAX = 4;


constexpr int IDM_WAIT_CYCLES = 100000;
constexpr int IDM_BARRIER_CYCLES = IDM_WAIT_CYCLES / 1000;

// [DATASET & ACCESS METHODS]
DRV_API_REF_CLASS_BEGIN(Vertex)
DRV_API_REF_CLASS_DATA_MEMBER(Vertex, id)
DRV_API_REF_CLASS_DATA_MEMBER(Vertex, edges)
DRV_API_REF_CLASS_DATA_MEMBER(Vertex, start)
DRV_API_REF_CLASS_DATA_MEMBER(Vertex, type)
DRV_API_REF_CLASS_END(Vertex)

inline Vertex readRef(const Vertex_ref &r) {
  return {r.id(), r.edges(), r.start(), r.type()};
}

DRV_API_REF_CLASS_BEGIN(Edge)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, src)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, dst)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, type)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, src_type)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, dst_type)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, src_glbid)
DRV_API_REF_CLASS_DATA_MEMBER(Edge, dst_glbid)
DRV_API_REF_CLASS_END(Edge)

inline Edge readRef(const Edge_ref &r) {
  return {r.src(), r.dst(), r.type(), r.src_type(), r.dst_type(), r.src_glbid(), r.dst_glbid()};
}

#define DRV_READ_STRUCT_DEF(type)                       \
  inline type read ## type (const DrvAPIPointer< type > &p, size_t pos) { \
    type ## _ref r = &p[pos];                                             \
    return readRef(r);                                        \
  }

DRV_READ_STRUCT_DEF(Vertex)
DRV_READ_STRUCT_DEF(Edge)

struct Data01CSR {
  DrvAPIPointer<Vertex> VArr;
  DrvAPIPointer<Edge> EArr;

  size_t VSize = 0, ESize = 0;

  Data01CSR(DrvAPIAddress addr) {
    // uint64_t imageAddr = DrvAPIVAddress::MainMemBase(0).encode() + 0x38000000;
    uint64_t imageAddr = addr;
    uint64_t VArrAddr = imageAddr;
    uint64_t EArrAddr = VArrAddr + 6349960;

    VSize = DrvAPI::read<uint64_t>(VArrAddr);
    ESize = DrvAPI::read<uint64_t>(EArrAddr);
    VArr = DrvAPIPointer<Vertex>(VArrAddr + 8);
    EArr = DrvAPIPointer<Edge>(EArrAddr + 8);
  }
};

struct CSRInterface {
  Data01CSR local, remote;
  uint64_t VArrSz, EArrSz;
  uint64_t VLocalAccessCnt = 0, VRemoteAccessCnt = 0;
  uint64_t ELocalAccessCnt = 0, ERemoteAccessCnt = 0;

  // the 6th bank is the local image
  // the 7th bank is modeled as remote image, with 1us access latency
  CSRInterface(unsigned lpxn, unsigned rpxn):
    local(DrvAPIVAddress::MainMemBase(lpxn).encode() + 0x30000000),
    remote(DrvAPIVAddress::MainMemBase(rpxn).encode() + 0x38000000)
  {
    VArrSz = local.VSize;
    EArrSz = local.ESize;
  }

  bool localVertexPos(size_t n) { return n < VArrSz / SIM_PXN;}
  bool localEdgePos(size_t n) { return n < EArrSz / SIM_PXN;}

  Vertex V(size_t n) {
    if (localVertexPos(n)) {
      VLocalAccessCnt++;
      return readVertex(local.VArr, n);
    } else {
      VRemoteAccessCnt++;
      return readVertex(remote.VArr, n);
    }
  }

  Edge E(size_t n) {
    if (localEdgePos(n)) {
      ELocalAccessCnt++;
      return readEdge(local.EArr, n);
    } else {
      ERemoteAccessCnt++;
      return readEdge(remote.EArr, n);
    }
  }
};

// [HELPER UTILITIES]
DrvAPIGlobalDRAM<int> g_barrier1, g_barrier2;
int totalThreads() {
  return myCoreThreads() * numPodCores() * numPXNPods();
}

int totalComputeThreads() {
  return totalThreads() / 2;
}

int myThread() {
  return myThreadId() + myCoreId() * (myCoreThreads());
}

bool isComputeThread() {
  return myCoreId() < numPodCores() / 2;
}
int myPairID() {
  return myThread() % totalComputeThreads();
}

// [IDM-COMP COMMUNICATION]
DrvAPIGlobalL2SP<DrvAPIPointer<int>> g_threadStatus;

// [CACHING]

/*
 * Semantics Enhanced Caching Scheme
 */

struct IDMVCache {
  DrvAPIPointer<Vertex> value_;
  DrvAPIPointer<uint32_t> arg1_; // arg1: id
  DrvAPIPointer<bool> lock_;
  size_t size_;

  IDMVCache(DrvAPIPointer<Vertex> value, DrvAPIPointer<uint32_t> arg1, DrvAPIPointer<bool> lock, size_t size)
    : value_(value), arg1_(arg1), lock_(lock), size_(size) { }

  bool lookup(uint32_t id, Vertex* vout) {
    int pos = id % size_;
    bool r = false;
    if (atomic_swap(lock_ + pos, true)) return false;

    if (arg1_[pos] == id) {
      *vout = readVertex(value_, pos);
      r = true;
    }

    lock_[pos] = false;
    return r;
  }

  bool write(uint32_t id, Vertex& vin) {
    int pos = id % size_;
    if (atomic_swap(lock_ + pos, true)) return false;

    arg1_[pos] = id;
    value_[pos] = vin;

    lock_[pos] = false;
    return true;
  }
};

struct IDMSamplingCache {
  constexpr static int MS = 5;          // maximum number of samples
  DrvAPIPointer<Edge> value_;
  DrvAPIPointer<uint32_t> arg1_; // arg1: id
  DrvAPIPointer<uint8_t> arg2_;  // arg2: sampling size
  DrvAPIPointer<bool> lock_;
  size_t size_;

  IDMSamplingCache(DrvAPIPointer<Edge> value,
    DrvAPIPointer<uint32_t> arg1, DrvAPIPointer<uint8_t> arg2,
    DrvAPIPointer<bool> lock, size_t size)
    : value_(value), arg1_(arg1), arg2_(arg2), lock_(lock), size_(size) { }

  bool lookup(uint32_t id, uint8_t sz, DrvAPIPointer<Edge> eout) {
    int pos = id % size_;
    bool r = false;
    if (atomic_swap(lock_ + pos, true)) return false;

    if (arg1_[pos] == id && arg2_[pos] == sz) {
      for (int i = 0; i < sz; i++) {
        eout[i] = readEdge(value_, pos * MS + i);
      }
      r = true;
    }
    lock_[pos] = false;
    return r;
  }

  bool write(uint32_t id, uint8_t sz, DrvAPIPointer<Edge> ein) {
    int pos = id % size_;
    if (atomic_swap(lock_ + pos, true)) return false;

    arg1_[pos] = id;
    arg2_[pos] = sz;
    for (int i = 0; i < sz; i++) {
      value_[pos * MS + i] = readEdge(ein, i);
    }

    lock_[pos] = false;
    return true;
  }
};

// resources
DrvAPIGlobalL2SP<DrvAPIPointer<Vertex>> g_idmVCacheValue;
DrvAPIGlobalL2SP<DrvAPIPointer<uint32_t>> g_idmVCacheArg1;
DrvAPIGlobalL2SP<DrvAPIPointer<bool>> g_idmVCacheLock;

DrvAPIGlobalL2SP<DrvAPIPointer<Edge>> g_idmSCacheValue;
DrvAPIGlobalL2SP<DrvAPIPointer<uint32_t>> g_idmSCacheArg1;
DrvAPIGlobalL2SP<DrvAPIPointer<uint8_t>> g_idmSCacheArg2;
DrvAPIGlobalL2SP<DrvAPIPointer<bool>> g_idmSCacheLock;

using IDMCacheA = IDMVCache;
using IDMCacheB = IDMSamplingCache;

/*
 * number of sample
 */
int NUM_SAMPLE[] = {5, 3, 2, 1, 0};
int MAX_NUM_NODE = 162; // 81
int MAX_NUM_EDGE = 256; // 162


void ComputeThread(CSRInterface &csr, IDMCacheA &idmVCache, IDMCacheB &idmECache) {
    uint64_t VArrSz = csr.VArrSz;
    uint64_t EArrSz = csr.EArrSz;
    /*
     * a model of ego graph generation kernel
     */
    size_t totalRoot = (VArrSz / 4) / SIM_PXN;
    size_t step = totalRoot / totalComputeThreads();
    size_t tid = myPairID();
    size_t beg = step * tid;
    size_t end = step * (tid + 1);
    if ((int) tid == totalComputeThreads() - 1) end = totalRoot;
    // small data structures should be allocated in L1SP
    // but L1SP is too small
    DrvAPIPointer<uint64_t> frontier
        = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_NODE);
    int frontier_head = 0, frontier_tail = 0;

    DrvAPIPointer<uint64_t> vertices
        = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_NODE);
    int vertices_size = 0;

    DrvAPIPointer<uint64_t> edgesSrc
        = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_EDGE);
    DrvAPIPointer<uint64_t> edgesDst
        = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_EDGE);
    int edges_size = 0;

    DrvAPIPointer<Edge> neighborhood
        = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(Edge) * 5);
    int neighborhood_size = 0;

    // statistics
    size_t sampledEdgeCnt = 0, sampledVertexCnt = 0;
    size_t localVCnt = 0, remoteVCnt = 0, hitVCnt = 0;
    size_t localECnt = 0, remoteECnt = 0, hitRemoteECnt = 0, hitLocalECnt = 0;
    size_t rootLocal = 0, rootRemote = 0;

    for (size_t i = beg; i < end; i++) {
      // if (myPairID() == 2) printf("status write %d\n", (int) i);
      // atomic_swap<int>(g_threadStatus[myPairID()], (int) i);
      g_threadStatus[myPairID()] = (int) i;
      // printf("[comp] ITERATION %d\n", (int) i);
      frontier[frontier_tail++] = i;
      vertices[vertices_size++] = i;
      edgesSrc[edges_size] = i;
      edgesDst[edges_size] = i;
      edges_size++;

      int next_level = 1;
      int level = 0;
      bool rootIt = true;
      while (frontier_head < frontier_tail) {
        uint64_t glbID = frontier[frontier_head];
        uint64_t V_localID = frontier_head;
        frontier_head++;
        // ![REMOTE/LOCAL]
        Vertex V;
        if (!csr.localVertexPos(glbID)) {
          remoteVCnt++;
          if (idmVCache.lookup(glbID, &V)) {
            hitVCnt++;
          } else {
            V = csr.V(glbID);
          }
        } else {
          localVCnt++;
          V = csr.V(glbID);
        }

        // Gather neighbors
        neighborhood_size = 0;

        uint64_t startEL = V.start;
        uint64_t endEL = startEL + V.edges;
        uint64_t num_neighbors = V.edges;

        uint64_t edges_to_fetch = std::min<uint64_t>(NUM_SAMPLE[level], num_neighbors);

        // should be offloaded to DMA
        if (edges_to_fetch) {
          if (csr.localEdgePos(startEL)) {
            localECnt += edges_to_fetch;
          } else {
            remoteECnt += edges_to_fetch;
          }
          if (idmECache.lookup(glbID, edges_to_fetch, neighborhood)) {
            neighborhood_size = edges_to_fetch;
            if (!csr.localEdgePos(startEL)) hitRemoteECnt += edges_to_fetch;
            else hitLocalECnt += edges_to_fetch;
          } else {
            for (unsigned int i = 0; i < edges_to_fetch; ++i) {
              size_t v = rand() % num_neighbors;
              // ![REMOTE/LOCAL]
              Edge e = csr.E(startEL + v);
              neighborhood[neighborhood_size++] = e;
            }
          }
        }

        for (int i = 0; i < neighborhood_size; i++) {
          Edge_ref n_erf = &neighborhood[i];
          uint64_t uGlbID = n_erf.dst_glbid();

          bool visited = false;
          int searched = -1;
          for (int j = 0; j < vertices_size; j++) {
            if (vertices[j] == uGlbID) {
              visited = true;
              searched = j;
              break;
            }
          }

          if (rootIt) {
            if (csr.localVertexPos(uGlbID)) {
              rootLocal++;
            } else {
              rootRemote++;
            }
          }

          if (!visited) {
            uint64_t U_localID = vertices_size;
            vertices[vertices_size++] = uGlbID;

            edgesSrc[edges_size] = U_localID;
            edgesDst[edges_size] = U_localID;
            edges_size++;

            edgesSrc[edges_size] = V_localID;
            edgesDst[edges_size] = U_localID;
            edges_size++;

            edgesSrc[edges_size] = U_localID;
            edgesDst[edges_size] = V_localID;
            edges_size++;

            frontier[frontier_tail++] = uGlbID;
          } else { // visited
            uint64_t U_localID = searched;

            edgesSrc[edges_size] = V_localID;
            edgesDst[edges_size] = U_localID;
            edges_size++;

            edgesSrc[edges_size] = U_localID;
            edgesDst[edges_size] = V_localID;
            edges_size++;
          }
        }


        if (frontier_head == next_level) {
          level++;
          next_level = frontier_tail;
        }
        rootIt = false;
      }

      sampledEdgeCnt += edges_size;
      sampledVertexCnt += vertices_size;

      // ignore post processing steps
      // clear all data structures
      edges_size = 0;
      vertices_size = 0;
      frontier_head = frontier_tail = 0;
      neighborhood_size = 0;

    }
    g_threadStatus[myPairID()] = (int) -1;

    #ifdef OUTPUT
    printf("=========================== Compute thread %4d done ===========================\n", myPairID());
    printf("work: %lu, sampled edges: %lu, sampled vertices: %lu\n",
      end - beg, sampledEdgeCnt, sampledVertexCnt);
    printf("avg sampled edges: %.2lf, avg sampled vertices: %.2lf\n",
      ((double) sampledEdgeCnt) / (end - beg),
      ((double) sampledVertexCnt) / (end - beg)
      );

    printf("V local accesses: %lu, V remote accesses: %lu (from IDM cache: %lu [%.2lf%%])\n",
      localVCnt, remoteVCnt, hitVCnt, ((double) hitVCnt/ remoteVCnt * 100.0));
    printf("E local accesses: %lu (from IDM cache: %lu [%.2lf%%])\nE remote accesses: %lu (from IDM cache: %lu [%.2lf%%])\n",
      localECnt, hitLocalECnt, ((double) hitLocalECnt / localECnt * 100.0),
      remoteECnt, hitRemoteECnt, ((double) hitRemoteECnt / remoteECnt * 100.0));
    printf("Root Local, Root Remote, %d, %d\n", (int) rootLocal, (int) rootRemote);
    printf("================================================================================\n");
    #endif
}

void IDMThread(CSRInterface &csr, IDMCacheA &idmVCache, IDMCacheB &idmECache) {
  {
    while (g_threadStatus[myPairID()] == 0) DrvAPI::wait(IDM_BARRIER_CYCLES);
  }

  // statistics
  int resetCnt = 0, waitCnt = 0;

  // prefetch start
  int cur = 0, compIt;
  DrvAPIPointer<uint64_t> frontier
      = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_NODE);
  int frontier_head = 0, frontier_tail = 0;

  DrvAPIPointer<uint64_t> vertices
      = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(uint64_t) * MAX_NUM_NODE);
  int vertices_size = 0;

  DrvAPIPointer<Edge> neighborhood
      = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, sizeof(Edge) * 5);
  int neighborhood_size = 0;

  while ((compIt = g_threadStatus[myPairID()]) != -1) {
    if (cur <= compIt + 1) {
      resetCnt++;
      cur = compIt + PREFETCH_AHEAD_MAX;
      continue;
    }
     else if (cur > compIt + PREFETCH_AHEAD_MAX) {
      waitCnt++;
      // [NOTE] do not know why, compIT can have out-of-air value
      cur = compIt + PREFETCH_AHEAD_MAX + 1;
      wait(IDM_WAIT_CYCLES);
      continue;
    }

    // prefetch logic

    frontier[frontier_tail++] = cur;
    vertices[vertices_size++] = cur;

    int next_level = 1;
    int level = 0;
    while (frontier_head < frontier_tail) {
      uint64_t glbID = frontier[frontier_head];
      frontier_head++;
      // ![REMOTE/LOCAL]
      Vertex V = csr.V(glbID);
      if (!csr.localVertexPos(glbID)) {
        idmVCache.write(glbID, V);
      }

      // Gather neighbors
      neighborhood_size = 0;

      uint64_t startEL = V.start;
      uint64_t endEL = startEL + V.edges;
      uint64_t num_neighbors = V.edges;

      uint64_t edges_to_fetch = std::min<uint64_t>(NUM_SAMPLE[level], num_neighbors);

      // [Semantics Enhanded Caching]
      for (unsigned int i = 0; i < edges_to_fetch; ++i) {
        // ![REMOTE/LOCAL]
        Edge e = csr.E(startEL + i);
        neighborhood[neighborhood_size++] = e;
      }
      if (edges_to_fetch) {
        idmECache.write(glbID, edges_to_fetch, neighborhood);
      }

      for (int i = 0; i < neighborhood_size; i++) {
        Edge_ref n_erf = &neighborhood[i];
        uint64_t uGlbID = n_erf.dst_glbid();

        bool visited = false;
        int searched = -1;
        for (int j = 0; j < vertices_size; j++) {
          if (vertices[j] == uGlbID) {
            visited = true;
            searched = j;
            break;
          }
        }

        if (!visited) {
          vertices[vertices_size++] = uGlbID;
          frontier[frontier_tail++] = uGlbID;
        }
      }


      if (frontier_head == next_level) {
        level++;
        next_level = frontier_tail;
      }
    }

    // clear all data structures
    vertices_size = 0;
    frontier_head = frontier_tail = 0;
    neighborhood_size = 0;

    cur++;
  }

  #ifdef OUTPUT
  printf("=========================== IDM thread for %4d done ===========================\n", myPairID());
  printf("number of reset: %20d, number of wait: %20d\n", resetCnt, waitCnt);
  printf("================================================================================\n");
  #endif
}

int AppMain(int argc, char *argv[])
{
  using namespace DrvAPI;

  if (myThreadId() == -1 && myCoreId() == -1) { return -1; }
  DrvAPIMemoryAllocatorInit();

  CSRInterface csr(0, 0);

  if (myThread() == 0) {

    // [Semantics Enhanced Caching]
    int numEntry = totalComputeThreads() * CACHE_SIZE;

    g_idmVCacheValue = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, numEntry * sizeof(Vertex));
    g_idmVCacheArg1 = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, numEntry * sizeof(uint32_t));
    g_idmVCacheLock = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, numEntry);

    g_idmSCacheValue = DrvAPIMemoryAlloc(DrvAPIMemoryDRAM, numEntry * sizeof(Edge) * IDMCacheB::MS);
    g_idmSCacheArg1 = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, numEntry * sizeof(uint32_t));
    g_idmSCacheArg2 = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, numEntry);
    g_idmSCacheLock = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, numEntry);

    g_threadStatus = DrvAPIMemoryAlloc(DrvAPIMemoryL2SP, totalComputeThreads());
  }

  if (isComputeThread()) {
    g_threadStatus[myPairID()] = 0;
  }

  DrvAPI::atomic_add<int>(&g_barrier1, 1);

  // barrier to wait until loading finishes
  {
    int t = totalThreads();
    while (g_barrier1 != t) DrvAPI::wait(1000);
  }

  // [Semantics Enhanced Caching]
  size_t cacheOffset = CACHE_SIZE * myPairID();

  auto idmVCacheValue = static_cast<DrvAPIPointer<Vertex>>(g_idmVCacheValue) + cacheOffset;
  auto idmVCacheArg1 = static_cast<DrvAPIPointer<uint32_t>>(g_idmVCacheArg1) + cacheOffset;
  auto idmVCacheLock = static_cast<DrvAPIPointer<bool>>(g_idmVCacheLock) + cacheOffset;

  auto idmSCacheValue = static_cast<DrvAPIPointer<Edge>>(g_idmSCacheValue) + cacheOffset * IDMCacheB::MS;
  auto idmSCacheArg1 = static_cast<DrvAPIPointer<uint32_t>>(g_idmSCacheArg1) + cacheOffset;
  auto idmSCacheArg2 = static_cast<DrvAPIPointer<uint8_t>>(g_idmSCacheArg2) + cacheOffset;
  auto idmSCacheLock = static_cast<DrvAPIPointer<bool>>(g_idmSCacheLock) + cacheOffset;

  IDMCacheA idmVCache(idmVCacheValue, idmVCacheArg1, idmVCacheLock, CACHE_SIZE);

  IDMCacheB idmECache(idmSCacheValue, idmSCacheArg1, idmSCacheArg2, idmSCacheLock, CACHE_SIZE);

  DrvAPI::atomic_add<int>(&g_barrier2, 1);

  {
    int t = totalThreads();
    while (g_barrier2 != t)  DrvAPI::wait(1000);
  }


  if (isComputeThread()) {
    ComputeThread(csr, idmVCache, idmECache);
  }
#ifdef WITH_IDM
  else {
    IDMThread(csr, idmVCache, idmECache);
  }
#endif

  return 0;
}
declare_drv_api_main(AppMain);
