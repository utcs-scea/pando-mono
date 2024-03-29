// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_INFO_H
#define DRV_API_INFO_H
#include <DrvAPISysConfig.hpp>
#include <DrvAPIThread.hpp>

namespace DrvAPI {

////////////////////
// Some constants //
////////////////////
static constexpr int CORE_ID_COMMAND_PROCESSOR = -1;

/////////////////////////////////
// Thread-Relative Information //
/////////////////////////////////
/**
 * return my thread id w.r.t my core
 */
inline int myThreadId() {
  return DrvAPIThread::current()->threadId();
}

/**
 * return my core id w.r.t my pod
 */
inline int myCoreId() {
  return DrvAPIThread::current()->coreId();
}

/**
 * return a core's x  w.r.t my pod
 */
inline int coreXFromId(int core) {
  return core & 7;
}

/**
 * return a core's y  w.r.t my pod
 */
inline int coreYFromId(int core) {
  return (core >> 3) & 7;
}

/**
 * return a core's id from its x y
 */
inline int coreIdFromXY(int x, int y) {
  return x + (y << 3);
}

/**
 * return my core's x  w.r.t my pod
 */
inline int myCoreX() {
  return coreXFromId(myCoreId());
}

/**
 * return my core's y  w.r.t my pod
 */
inline int myCoreY() {
  return coreYFromId(myCoreId());
}

/**
 * return true if I am the command processor
 */
inline bool isCommandProcessor() {
  return myCoreId() == CORE_ID_COMMAND_PROCESSOR;
}

/**
 * return my pod id w.r.t my pxn
 */
inline int myPodId() {
  return DrvAPIThread::current()->podId();
}

/**
 * return my pxn id
 */
inline int myPXNId() {
  return DrvAPIThread::current()->pxnId();
}

inline int myCoreThreads() {
  return DrvAPIThread::current()->coreThreads();
}

//////////////////////
// System Constants //
//////////////////////
inline int numPXNs() {
  return DrvAPISysConfig::Get()->numPXN();
}

inline int numPXNPods() {
  return DrvAPISysConfig::Get()->numPXNPods();
}

inline int numPodCores() {
  return DrvAPISysConfig::Get()->numPodCores();
}

inline int numCoreThreads() {
  return DrvAPISysConfig::Get()->numCoreThreads();
}

/**
 * size of l1sp in bytes
 */
inline uint64_t coreL1SPSize() {
  return DrvAPISysConfig::Get()->coreL1SPSize();
}

/**
 * size of l2sp in bytes
 */
inline uint64_t podL2SPSize() {
  return DrvAPISysConfig::Get()->podL2SPSize();
}

/**
 * size of pxn's dram in bytes
 */
inline uint64_t pxnDRAMSize() {
  return DrvAPISysConfig::Get()->pxnDRAMSize();
}

/**
 * number of dram ports
 */
inline int numPXNDRAMPorts() {
  return DrvAPISysConfig::Get()->pxnDRAMPortCount();
}

/**
 * size of the address interleave for dram
 */
inline uint64_t pxnDRAMAddressInterleave() {
  return DrvAPISysConfig::Get()->pxnDRAMInterleaveSize();
}

/**
 * number of pod l2sp banks
 */
inline int32_t numPodL2SPBanks() {
  return DrvAPISysConfig::Get()->podL2SPBankCount();
}

inline uint32_t podL2SPAddressInterleave() {
  return DrvAPISysConfig::Get()->podL2SPInterleaveSize();
}
//////////
// Time //
//////////
/**
 * return the cycle count
 */
inline uint64_t cycle() {
  return DrvAPIThread::current()->getSystem()->getCycleCount();
}

/**
 * return the hz
 */
inline uint64_t HZ() {
  return DrvAPIThread::current()->getSystem()->getClockHz();
}

inline double seconds() {
  return DrvAPIThread::current()->getSystem()->getSeconds();
}

inline double picoseconds() {
  return seconds() * 1e12;
}

/**
 * this will force the simulator to do a global statistics dump
 */
inline void outputStatistics() {
  DrvAPIThread::current()->getSystem()->outputStatistics("none");
}

/**
 * this will force the simulator to do a global statistics dump with a tag
 */
inline void outputStatistics(const std::string& tag) {
  DrvAPIThread::current()->getSystem()->outputStatistics(tag);
}

} // namespace DrvAPI
#endif
