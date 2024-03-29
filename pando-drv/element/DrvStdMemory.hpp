// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#pragma once
#include <DrvAPIAddress.hpp>
#include "DrvMemory.hpp"
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/event.h>
#include <sst/elements/memHierarchy/memoryController.h>
#include <atomic>
#include <cmath>
namespace SST {
namespace Drv {
/**
 * @brief Memory model that uses sst standard memory
 *
 */
class DrvStdMemory : public DrvMemory {
public:
    /**
     * @brief record type for mapping address ranges to mem controllers
     */
    typedef std::tuple<uint64_t, uint64_t, SST::MemHierarchy::MemController*> record_type;

    /**
     * @brief Holds data to help with toNative function
     */
    struct ToNativeMetaData {
        /**
         * helps decode and adddress
         */
        struct InterleaveDecoder {
            InterleaveDecoder() = default;
            InterleaveDecoder(uint64_t interleave, int64_t banks) {
                offset_mask = interleave - 1;
                bank_shift = static_cast<uint64_t>(std::log2(interleave));
                bank_mask = banks - 1;
                segment_shift = bank_shift + static_cast<uint64_t>(std::log2(banks));
            }
            uint64_t offset_mask = 0;
            uint64_t bank_shift = 0;
            uint64_t bank_mask = 0;
            uint64_t segment_shift = 0;
            /**
             * @brief Get the Segment Bank Offset from an address
             *
             * @brief addr is an offset address from an interleaved memory range
             */
            std::tuple<uint64_t, uint64_t>
            getBankOffset(uint64_t addr) const {
                uint64_t bank = (addr >> bank_shift) & bank_mask;
                uint64_t offset = addr & offset_mask;
                return std::make_tuple(bank, offset);
            }
        };
        ToNativeMetaData() = default;
        ToNativeMetaData(const ToNativeMetaData&) = delete;
        ToNativeMetaData& operator=(const ToNativeMetaData&) = delete;
        ToNativeMetaData(ToNativeMetaData&&) = delete;
        ToNativeMetaData& operator=(ToNativeMetaData&&) = delete;
        ~ToNativeMetaData() = default;

        /**
         * initialize records
         */
        void init(DrvStdMemory *mem);

        std::vector<std::vector<std::vector<record_type>>> l1sp_mcs; //!< l1sps mem controllers and their address ranges
        std::vector<std::vector<std::vector<record_type>>> l2sp_mcs; //!< l2sps mem controllers and their address ranges
        std::vector<std::vector<record_type>> dram_mcs; //!< drams mem controllers and their address ranges
        InterleaveDecoder l2sp_interleave_decode;
        InterleaveDecoder dram_interleave_decode;
        std::atomic<bool> initialized = false;
    };

    friend class ToNativeMetaData;

    // register this subcomponent into the element library
    SST_ELI_REGISTER_SUBCOMPONENT(
        SST::Drv::DrvStdMemory,
        "Drv",
        "DrvStdMemory",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Memory that interfaces with memHierarchy components",
        SST::Drv::DrvMemory
    )

    // register subcomponent slots
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"memory", "Memory component", "SST::Interfaces::StandardMem"}
    )

    /**
     * @brief Construct a new DrvStdMemory object
     *
     * @param id
     * @param params
     * @param core
     */
    DrvStdMemory(SST::ComponentId_t id, SST::Params& params, DrvCore *core);

    /**
     * @brief Destroy the DrvStdMemory object
     *
     */
    virtual ~DrvStdMemory();

    /**
     * @brief Send a memory request
     *
     * @param core
     * @param thread
     * @param mem_req
     */
    void sendRequest(DrvCore *core
                     ,DrvThread *thread
                     ,const std::shared_ptr<DrvAPI::DrvAPIMem> &mem_req);

    /**
     * @brief init is called at the beginning of the simulation
     */
    void init(unsigned int phase);

    /**
     * @brief setup is called during setup phase
     */
    void setup();

    /**
     * @brief finish is called at the end of the simulation
     */
    void finish() { mem_->finish(); }

    /**
     * @brief translate a pgas pointer to a native pointer
     */
    void toNativePointer(DrvAPI::DrvAPIAddress addr, void **ptr, size_t *size);

private:
    /**
     * @brief handleEvent is called when a message is received
     *
     * @param req
     */
    void handleEvent(SST::Interfaces::StandardMem::Request *req);

    /**
     * @brief translate a pgas pointer to a native pointer
     */
    void toNativePointerDRAM(DrvAPI::DrvAPIAddress addr, void **ptr, size_t *size);

    /**
     * @brief translate a pgas pointer to a native pointer
     */
    void toNativePointerL2SP(DrvAPI::DrvAPIAddress addr, void **ptr, size_t *size);

    /**
     * @brief translate a pgas pointer to a native pointer
     */
    void toNativePointerL1SP(DrvAPI::DrvAPIAddress addr, void **ptr, size_t *size);

    Interfaces::StandardMem *mem_; //!< The memory

    static ToNativeMetaData to_native_meta_data_; //!< holds data to help with toNative function
};

}
}
