// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include "DrvAPIGlobal.hpp"
#include <array>
#include <sstream>
namespace DrvAPI {

/**
 * @brief a section for which the software address is constant no matter what core
 * you are on since it uses core-relative addressing
 */
class CoreRelativeAddressingSection : public DrvAPISection {
public:
  CoreRelativeAddressingSection() : base_(0) {}
  CoreRelativeAddressingSection(const CoreRelativeAddressingSection&) = delete;
  CoreRelativeAddressingSection& operator=(const CoreRelativeAddressingSection&) = delete;
  CoreRelativeAddressingSection(CoreRelativeAddressingSection&&) = delete;
  CoreRelativeAddressingSection& operator=(CoreRelativeAddressingSection&&) = delete;
  ~CoreRelativeAddressingSection() = default;

  /**
   * @brief get the section base
   */
  uint64_t getBase(uint32_t pxn, uint32_t pod, uint32_t core) const override {
    return base_;
  }

  /**
   * @brief set the section base
   */
  void setBase(uint64_t base, uint32_t pxn, uint32_t pod, uint32_t core) override {
    base_ = base;
  }

private:
  std::atomic<uint64_t> base_; //!< core-relative base address
};

/**
 * @brief a section whose base pointer is a function of its pxn
 */
class PXNDependentBaseSection : public DrvAPISection {
public:
  PXNDependentBaseSection() {
    for (auto& b : base_) {
      b = 0;
    }
  }
  PXNDependentBaseSection(const PXNDependentBaseSection&) = delete;
  PXNDependentBaseSection& operator=(const PXNDependentBaseSection&) = delete;
  PXNDependentBaseSection(PXNDependentBaseSection&&) = delete;
  PXNDependentBaseSection& operator=(PXNDependentBaseSection&&) = delete;

  /**
   * @brief get the section base
   */
  uint64_t getBase(uint32_t pxn, uint32_t pod, uint32_t core) const override {
    checkPXN(pxn);
    return base_[pxn];
  }

  /**
   * @brief set the section base
   */
  void setBase(uint64_t base, uint32_t pxn, uint32_t pod, uint32_t core) override {
    checkPXN(pxn);
    base_[pxn] = base;
  }

private:
  void checkPXN(uint32_t pxn) const {
    if (pxn >= base_.size()) {
      std::stringstream ss;
      ss << "PXNDependentBaseSection::checkPxn:";
      ss << "pxn out of range: " << pxn;
      ss << " base_.size() = " << base_.size();
      throw std::runtime_error(ss.str());
    }
  }

  /* we will throw an error if more than 1024 pxns are simulaled */
  std::array<std::atomic<uint64_t>, 1024> base_; //!< base address for each pxn
};

__attribute__((init_priority(101))) static CoreRelativeAddressingSection l1sp;
__attribute__((init_priority(101))) static CoreRelativeAddressingSection l2sp;
__attribute__((init_priority(101))) static PXNDependentBaseSection dram;

DrvAPISection& DrvAPISection::GetSection(DrvAPIMemoryType memtype) {
  switch (memtype) {
    case DrvAPIMemoryType::DrvAPIMemoryL1SP:
      return l1sp;
    case DrvAPIMemoryType::DrvAPIMemoryL2SP:
      return l2sp;
    case DrvAPIMemoryType::DrvAPIMemoryDRAM:
      return dram;
    default:
      throw std::runtime_error("Invalid memory type");
  }
}

} // namespace DrvAPI
