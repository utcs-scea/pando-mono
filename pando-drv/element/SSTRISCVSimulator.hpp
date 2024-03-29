// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include "DrvNativeSimulationTranslator.hpp"
#include <DrvAPIAddress.hpp>
#include <DrvAPIReadModifyWrite.hpp>
#include <RV64IMFInterpreter.hpp>
#include <functional>
#include <map>
#include <sst/core/interfaces/stdMem.h>
namespace SST {
namespace Drv {

class RISCVCore;
class RISCVSimHart;

/**
 * @brief a riscv simulator
 */
class RISCVSimulator : public RV64IMFInterpreter {
public:
  /**
   * constructor
   */
  RISCVSimulator(RISCVCore* core) : core_(core) {}

  /**
   * destructor
   */
  virtual ~RISCVSimulator() {}

  // load/stores
  void visitLB(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLH(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLBU(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLHU(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLWU(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitFLW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitLD(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitSB(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitSH(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitSW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitSD(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitFENCE(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitFSW(RISCVHart& hart, RISCVInstruction& instruction) override;

  // csr instructions
private:
  uint64_t visitCSRRWUnderMask(RISCVHart& hart, uint64_t csr, uint64_t wval, uint64_t mask);

public:
  void visitCSRRW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitCSRRS(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitCSRRC(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitCSRRWI(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitCSRRSI(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitCSRRCI(RISCVHart& hart, RISCVInstruction& instruction) override;

  // atomics
private:
  template <typename T>
  void visitAMO(RISCVHart& hart, RISCVInstruction& i, DrvAPI::DrvAPIMemAtomicType op);

  void visitAMOSWAPW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPW_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPW_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPW_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  void visitAMOADDW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDW_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDW_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDW_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  void visitAMOSWAPD(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPD_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPD_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOSWAPD_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  void visitAMOADDD(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDD_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDD_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOADDD_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  template <typename T>
  void visitAMOCAS(RISCVHart& hart, RISCVInstruction& i);

  void visitAMOCASW(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASW_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASW_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASW_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  void visitAMOCASD(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASD_RL(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASD_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;
  void visitAMOCASD_RL_AQ(RISCVHart& hart, RISCVInstruction& instruction) override;

  // environment calls
  void visitECALL(RISCVHart& hart, RISCVInstruction& instruction) override;

  RISCVCore* core_; //!< the riscv core component
  static constexpr uint64_t MMIO_SIZE = 0xFFFF;
  static constexpr uint64_t MMIO_BASE = 0xFFFFFFFFFFFF0000;
  static constexpr uint64_t MMIO_PRINT_INT = MMIO_BASE + 0x0000;
  static constexpr uint64_t MMIO_PRINT_HEX = MMIO_BASE + 0x0008;
  static constexpr uint64_t MMIO_PRINT_CHAR = MMIO_BASE + 0x0010;
  static constexpr uint64_t MMIO_PRINT_TIME = MMIO_BASE + 0x0018;

  // CSRs
  static constexpr uint64_t CSR_MHARTID = 0xF14;
  static constexpr uint64_t CSR_MCOREID = 0xF15;
  static constexpr uint64_t CSR_MPODID = 0xF16;
  static constexpr uint64_t CSR_MPXNID = 0xF17;
  static constexpr uint64_t CSR_MCOREHARTS = 0xF18;
  static constexpr uint64_t CSR_MPODCORES = 0xF19;
  static constexpr uint64_t CSR_MPXNPODS = 0xF1A;
  static constexpr uint64_t CSR_MNUMPXN = 0xF1B;
  static constexpr uint64_t CSR_MCOREL1SPSIZE = 0xF1C;
  static constexpr uint64_t CSR_MPODL2SPSIZE = 0xF1D;
  static constexpr uint64_t CSR_MPXNDRAMSIZE = 0xF1E;
  static constexpr uint64_t CSR_MSTATUS = 0x300;

  static constexpr uint64_t CSR_FRM = 0x002;
  static constexpr uint64_t CSR_MIE = 0x304;   // interrupt enable
  static constexpr uint64_t CSR_MTVEC = 0x305; // where to jump on trap
  static constexpr uint64_t CSR_MEPC = 0x341;  // where to jump on exception
  static constexpr uint64_t CSR_CYCLE = 0xC00;

private:
  /**
   * @brief The LargeReadHandler class
   * coealesces multiple read requests into a single callback
   */
  class LargeReadHandler {
  public:
    using ResponseType = SST::Interfaces::StandardMem::ReadResp;
    LargeReadHandler(size_t n_requests, std::function<void(std::vector<uint8_t>&)>&& completion)
        : n_requests(n_requests), completion(completion) {
      responses.reserve(n_requests);
    }
    virtual ~LargeReadHandler() {
      for (ResponseType* rsp : responses) {
        delete rsp;
      }
    }
    void recvRsp(SST::Interfaces::StandardMem::Request* req) {
      ResponseType* rsp = dynamic_cast<ResponseType*>(req);
      if (rsp == nullptr) {
        SST::Output::getDefaultObject().fatal(
            CALL_INFO, -1, "LargeReadHandler: received a response of the wrong type\n");
      }
      responses.push_back(rsp);
      if (responses.size() == n_requests) {
        // 1.. sort responses by address
        std::sort(responses.begin(), responses.end(), [](ResponseType* a, ResponseType* b) {
          return a->pAddr < b->pAddr;
        });
        std::vector<uint8_t> data;
        for (ResponseType* rsp : responses) {
          data.insert(data.end(), rsp->data.begin(), rsp->data.end());
        }
        completion(data);
      }
    }
    size_t n_requests;
    std::vector<ResponseType*> responses;
    std::function<void(std::vector<uint8_t>&)> completion;
  };

  /**
   * @brief LargeWriteHandler
   * coalesces multiple write responses into a single callback
   */
  class LargeWriteHandler {
  public:
    using ResponseType = SST::Interfaces::StandardMem::WriteResp;
    LargeWriteHandler(size_t n_requests, std::function<void()>&& completion)
        : n_requests(n_requests), completion(completion) {
      responses.reserve(n_requests);
    }
    virtual ~LargeWriteHandler() {
      for (ResponseType* rsp : responses) {
        delete rsp;
      }
    }
    void recvRsp(SST::Interfaces::StandardMem::Request* req) {
      ResponseType* rsp = dynamic_cast<ResponseType*>(req);
      if (rsp == nullptr) {
        SST::Output::getDefaultObject().fatal(
            CALL_INFO, -1, "LargeWriteHandler: received a response of the wrong type\n");
      }
      responses.push_back(rsp);
      if (responses.size() == n_requests) {
        completion();
      }
    }
    size_t n_requests;
    std::vector<ResponseType*> responses;
    std::function<void()> completion;
  };

  template <typename R, typename T>
  void visitLoad(RISCVHart& hart, RISCVInstruction& instruction);

  template <typename T>
  void visitStore(RISCVHart& hart, RISCVInstruction& instruction);

  template <typename T>
  void visitStoreMMIO(RISCVHart& shart, RISCVInstruction& i);

  // TODO: implement this for malloc/free
  // or provide a different malloc/free implementation...
  void sysBRK(RISCVSimHart& shart, RISCVInstruction& i);

  void sysOPEN(RISCVSimHart& shart, RISCVInstruction& i);
  void sysWRITE(RISCVSimHart& shart, RISCVInstruction& i);
  void sysREAD(RISCVSimHart& shart, RISCVInstruction& i);
  void sysEXIT(RISCVSimHart& shart, RISCVInstruction& i);
  void sysReadBuffer(RISCVSimHart& shart, SST::Interfaces::StandardMem::Addr paddr, size_t n,
                     std::function<void(std::vector<uint8_t>&)>&& cont);
  void sysWriteBuffer(RISCVSimHart& shart, SST::Interfaces::StandardMem::Addr paddr,
                      std::vector<uint8_t>& data, std::function<void(void)>&& cont);

  // TODO: implement these for stdio

  // void sysREADV(RISCVSimHart &shart, RISCVInstruction &i);
  // void sysWRITEV(RISCVSimHart &shart, RISCVInstruction &i);
  void sysCLOSE(RISCVSimHart& shart, RISCVInstruction& i);
  void sysFSTAT(RISCVSimHart& shart, RISCVInstruction& i);

  bool isMMIO(SST::Interfaces::StandardMem::Addr addr);

  std::map<uint64_t, int64_t> _pchist;
  DrvNativeSimulationTranslator _type_translator;
};

} // namespace Drv
} // namespace SST
