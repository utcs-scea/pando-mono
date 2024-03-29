// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_SYS_CONFIG_HPP_
#define DRV_API_SYS_CONFIG_HPP_
#include <cstdint>
namespace DrvAPI
{

/**
 * @brief The DrvAPISysConfigData struct
 * This struct is used to store system configuration data.
 */
struct DrvAPISysConfigData
{
    int64_t num_pxn_; //!< number of PXNs in the system
    int64_t pxn_pods_; //!< number of pods per PXN
    int64_t pod_cores_; //!< number of cores per pod
    int64_t core_threads_; //!< number of threads per core
    int16_t nw_flit_dwords_; //!< number of dwords in a flit
    int16_t nw_obuf_dwords_; //!< number of dwords in an output buffer
    uint64_t core_l1sp_size_; //!< size of the L1 scratchpad
    uint64_t pod_l2sp_size_; //!< size of the L2 scratchpad
    uint64_t pxn_dram_size_; //!< size of the PXN DRAM
    int32_t  pxn_dram_ports_; //!< number of banks in the PXN DRAM
    uint32_t  pxn_dram_interleave_size_; //!< size of the address interleave in the PXN DRAM
    int32_t  pod_l2sp_banks_; //!< number of banks in the PXN L2 scratchpad
    uint32_t  pod_l2sp_interleave_size_; //!< size of the address interleave in the PXN L2 scratchpad
};


/**
 * @brief The DrvAPISysConfig class
 * This class is used to retrieve system configuration data.
 */
class DrvAPISysConfig
{
public:
    DrvAPISysConfig(const DrvAPISysConfigData& data): data_(data) {}
    DrvAPISysConfig() = default;
    ~DrvAPISysConfig() = default;
    DrvAPISysConfig(const DrvAPISysConfig&) = default;
    DrvAPISysConfig& operator=(const DrvAPISysConfig&) = default;
    DrvAPISysConfig(DrvAPISysConfig&&) = default;
    DrvAPISysConfig& operator=(DrvAPISysConfig&&) = default;

    int64_t numPXN() const { return data_.num_pxn_; }
    int64_t numPXNPods() const { return data_.pxn_pods_; }
    int64_t numPodCores() const { return data_.pod_cores_; }
    int64_t numCoreThreads() const { return data_.core_threads_; }
    int16_t numNWFlitDwords() const { return data_.nw_flit_dwords_; }
    int16_t numNWObufDwords() const { return data_.nw_obuf_dwords_; }
    uint64_t coreL1SPSize() const { return data_.core_l1sp_size_; }
    uint64_t podL2SPSize() const { return data_.pod_l2sp_size_; }
    uint64_t pxnDRAMSize() const { return data_.pxn_dram_size_; }
    int32_t pxnDRAMPortCount() const { return data_.pxn_dram_ports_; }
    uint32_t pxnDRAMInterleaveSize() const { return data_.pxn_dram_interleave_size_; }
    int32_t podL2SPBankCount() const { return data_.pod_l2sp_banks_; }
    uint32_t podL2SPInterleaveSize() const { return data_.pod_l2sp_interleave_size_; }

    static DrvAPISysConfig *Get() { return &sysconfig; }
    static DrvAPISysConfig sysconfig;
private:
    DrvAPISysConfigData data_;
};
}

/**
 * @brief DrvAPIGetSysConfig
 * @return the system configuration
 */
extern "C" DrvAPI::DrvAPISysConfig* DrvAPIGetSysConfig();

/**
 * @brief DrvAPIGetSysConfig_t
 */
typedef DrvAPI::DrvAPISysConfig* (*DrvAPIGetSysConfig_t)();

/**
 * @brief DrvAPISetSysConfig
 * @param sys_config the system configuration
 */
extern "C" void DrvAPISetSysConfig(DrvAPI::DrvAPISysConfig*);

/**
 * @brief DrvAPISetSysConfig_t
 */
typedef void (*DrvAPISetSysConfig_t)(DrvAPI::DrvAPISysConfig*);


#endif
