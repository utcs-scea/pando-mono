// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_SYS_CONFIG_HPP_
#define DRV_API_SYS_CONFIG_HPP_
#include <cstdint>
#include <vector>
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
 * @brief The DrvAPISysControlVariable struct
 * This struct is used to store system variables that control turning on and off the system.
 */
struct DrvAPISysControlVariable
{
    int64_t global_cps_finalized_; //!< global number of cps finalized
    int64_t global_cps_reached_; //!< global number of cps that reached the barrier
    std::vector<int64_t> pxn_cores_initialized_; //!< number of cores initialized per pxn
    std::vector<int8_t> pxn_barrier_exit_; //!< permission to exit the barrier for each pxn
    std::vector<std::vector<int64_t>> pod_cores_finalized_; //!< number of cores finalized per pod
    std::vector<std::vector<std::vector<int8_t>>> core_state_; //!< state for each core
    std::vector<std::vector<std::vector<int64_t>>> core_harts_done_; //!< number of harts done per core
};


/**
 * @brief The DrvAPISysConfig class
 * This class is used to retrieve system configuration data.
 */
class DrvAPISysConfig
{
public:
    DrvAPISysConfig(const DrvAPISysConfigData& data): data_(data) {
        variable_.global_cps_finalized_ = 0;
        variable_.global_cps_reached_ = 0;
        variable_.pxn_cores_initialized_.resize(data.num_pxn_);
        variable_.pxn_barrier_exit_.resize(data.num_pxn_);
        variable_.pod_cores_finalized_.resize(data.num_pxn_);
        variable_.core_state_.resize(data.num_pxn_);
        variable_.core_harts_done_.resize(data.num_pxn_);
        for (int i = 0; i < data_.num_pxn_; i++) {
            variable_.pxn_cores_initialized_[i] = 0;
            variable_.pxn_barrier_exit_[i] = 0;
            variable_.pod_cores_finalized_[i].resize(data.pxn_pods_);
            variable_.core_state_[i].resize(data.pxn_pods_);
            variable_.core_harts_done_[i].resize(data.pxn_pods_);
            for (int j = 0; j < data_.pxn_pods_; j++) {
                variable_.pod_cores_finalized_[i][j] = 0;
                variable_.core_state_[i][j].resize(data.pod_cores_);
                variable_.core_harts_done_[i][j].resize(data.pod_cores_);
                for (int k = 0; k < data_.pod_cores_; k++) {
                    variable_.core_state_[i][j][k] = 0;
                    variable_.core_harts_done_[i][j][k] = 0;
                }
            }
        }
    }
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

    void resetGlobalCpsFinalized() {
        __atomic_store_n(&(variable_.global_cps_finalized_), 0, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t atomicIncrementGlobalCpsFinalized(int64_t value) {
        return __atomic_fetch_add(&(variable_.global_cps_finalized_), value, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t getGlobalCpsFinalized() {
        return __atomic_load_n(&(variable_.global_cps_finalized_), static_cast<int>(std::memory_order_relaxed));
    }

    void resetGlobalCpsReached() {
        __atomic_store_n(&(variable_.global_cps_reached_), 0, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t atomicIncrementGlobalCpsReached(int64_t value) {
        return __atomic_fetch_add(&(variable_.global_cps_reached_), value, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t getGlobalCpsReached() {
        return __atomic_load_n(&(variable_.global_cps_reached_), static_cast<int>(std::memory_order_relaxed));
    }

    void resetPxnCoresInitialized(int64_t pxn_id) {
        __atomic_store_n(&(variable_.pxn_cores_initialized_.at(pxn_id)), 0, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t atomicIncrementPxnCoresInitialized(int64_t pxn_id, int64_t value) {
        return __atomic_fetch_add(&(variable_.pxn_cores_initialized_.at(pxn_id)), value, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t getPxnCoresInitialized(int64_t pxn_id) {
        return __atomic_load_n(&(variable_.pxn_cores_initialized_.at(pxn_id)), static_cast<int>(std::memory_order_relaxed));
    }

    void resetPxnBarrierExit(int64_t pxn_id) {
        __atomic_store_n(&(variable_.pxn_barrier_exit_.at(pxn_id)), 0, static_cast<int>(std::memory_order_relaxed));
    }
    void setPxnBarrierExit(int64_t pxn_id) {
        __atomic_store_n(&(variable_.pxn_barrier_exit_.at(pxn_id)), 1, static_cast<int>(std::memory_order_relaxed));
    }
    bool testPxnBarrierExit(int64_t pxn_id) {
        int8_t value = __atomic_load_n(&(variable_.pxn_barrier_exit_.at(pxn_id)), static_cast<int>(std::memory_order_relaxed));
        return value == 1;
    }

    void resetPodCoresFinalized(int64_t pxn_id, int8_t pod_id) {
        __atomic_store_n(&(variable_.pod_cores_finalized_.at(pxn_id).at(pod_id)), 0, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t atomicIncrementPodCoresFinalized(int64_t pxn_id, int8_t pod_id, int64_t value) {
        return __atomic_fetch_add(&(variable_.pod_cores_finalized_.at(pxn_id).at(pod_id)), value, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t getPodCoresFinalized(int64_t pxn_id, int8_t pod_id) {
        return __atomic_load_n(&(variable_.pod_cores_finalized_.at(pxn_id).at(pod_id)), static_cast<int>(std::memory_order_relaxed));
    }

    int8_t getCoreState(int64_t pxn_id, int8_t pod_id, int8_t core_id) {
        return __atomic_load_n(&(variable_.core_state_.at(pxn_id).at(pod_id).at(core_id)), static_cast<int>(std::memory_order_relaxed));
    }
    void setCoreState(int64_t pxn_id, int8_t pod_id, int8_t core_id, int8_t value) {
        __atomic_store_n(&(variable_.core_state_.at(pxn_id).at(pod_id).at(core_id)), value, static_cast<int>(std::memory_order_relaxed));
    }
    int8_t atomicCompareExchangeCoreState(int64_t pxn_id, int8_t pod_id, int8_t core_id, int8_t expected, int8_t desired) {
        int8_t expected_temp = expected;
        __atomic_compare_exchange_n(&(variable_.core_state_.at(pxn_id).at(pod_id).at(core_id)), &expected_temp, desired, false, static_cast<int>(std::memory_order_relaxed), static_cast<int>(std::memory_order_relaxed));
        return expected_temp;
    }

    void resetCoreHartsDone(int64_t pxn_id, int8_t pod_id, int8_t core_id) {
        __atomic_store_n(&(variable_.core_harts_done_.at(pxn_id).at(pod_id).at(core_id)), 0, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t atomicIncrementCoreHartsDone(int64_t pxn_id, int8_t pod_id, int8_t core_id, int64_t value) {
        return __atomic_fetch_add(&(variable_.core_harts_done_.at(pxn_id).at(pod_id).at(core_id)), value, static_cast<int>(std::memory_order_relaxed));
    }
    int64_t getCoreHartsDone(int64_t pxn_id, int8_t pod_id, int8_t core_id) {
        return __atomic_load_n(&(variable_.core_harts_done_.at(pxn_id).at(pod_id).at(core_id)), static_cast<int>(std::memory_order_relaxed));
    }

    static DrvAPISysConfig *Get() { return &sysconfig; }
    static DrvAPISysConfig sysconfig;
private:
    DrvAPISysConfigData data_;
    DrvAPISysControlVariable variable_;
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
