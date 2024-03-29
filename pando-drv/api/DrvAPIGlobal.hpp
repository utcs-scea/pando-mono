// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_GLOBAL_H
#define DRV_API_GLOBAL_H
#include <DrvAPIPointer.hpp>
#include <DrvAPIInfo.hpp>
#include <atomic>
#include <cassert>
namespace DrvAPI
{

class DrvAPISection
{
public:
    /**
     * @brief constructor
     */
    DrvAPISection() : size_(0) {}

    DrvAPISection(const DrvAPISection &other) = delete;
    DrvAPISection(DrvAPISection &&other) = delete;
    DrvAPISection &operator=(const DrvAPISection &other) = delete;
    DrvAPISection &operator=(DrvAPISection &&other) = delete;
    virtual ~DrvAPISection() = default;

    /**
     * @brief get the section base
     */
    virtual uint64_t
    getBase(uint32_t pxn, uint32_t pod, uint32_t core) const = 0;

    /**
     * @brief set the section base
     */
    virtual void
    setBase(uint64_t base, uint32_t pxn, uint32_t pod, uint32_t core) = 0;

    /**
     * @brief get the section size
     */
    uint64_t getSize() const { return size_; };

    /**
     * @brief set the section size
     */
    void setSize(uint64_t size) { size_ = size; }

    /**
     * @brief increase the section size by specified bytes
     *
     * @return the section size previous to resizing
     */
    uint64_t increaseSizeBy(uint64_t incr_size) {
        // incr_size should be 8-byte aligned
        incr_size = (incr_size + 7) & ~7;
        return size_.fetch_add(incr_size);
    }

    /**
     * @brief get the section of the specified memory type
     */
    static DrvAPISection &
    GetSection(DrvAPIMemoryType memtype);

protected:
    std::atomic<uint64_t> size_; //!< size of the section
};

/**
 * @brief Statically allocate data in the specified memory type
 */
template <typename T, DrvAPIMemoryType MEMTYPE>
class DrvAPIGlobal
{
public:
    typedef T value_type; //!< value type
    typedef DrvAPIPointer<T> pointer_type; //!< pointer type

    /**
     * @brief constructor
     */
    DrvAPIGlobal() {
        initOffset();
    }

    // not copyable or movable
    DrvAPIGlobal(const DrvAPIGlobal &other) = delete;
    DrvAPIGlobal(DrvAPIGlobal &&other) = delete;
    DrvAPIGlobal &operator=(const DrvAPIGlobal &other) = delete;
    DrvAPIGlobal &operator=(DrvAPIGlobal &&other) = delete;

    void initOffset() {
        DrvAPISection &section = DrvAPISection::GetSection(MEMTYPE);
        offset_ = section.increaseSizeBy(sizeof(T));
    }

    DrvAPIPointer<T> pointer() const {
        DrvAPIAddress base
            = DrvAPISection::GetSection(MEMTYPE)
            .getBase(myPXNId(), myPodId(), myCoreId());

        return DrvAPIPointer<T>(base + offset_);
    }

    /**
     * cast operator
     */
    operator T() const { return static_cast<T>(*pointer()); }

    /**
     * assignment operator
     */
    DrvAPIGlobal& operator=(const T &other) {
        *pointer() = other;
        return *this;
    }

    /**
     * address operator
     */
    DrvAPIPointer<T> operator&() const { return pointer(); }

    uint64_t offset_; //!< offset from the section base
    T      init_val_; //!< initial value
};

/**
 * @brief specialization for DrvAPIPointer globals
 */
template <typename T, DrvAPIMemoryType MEMTYPE>
class DrvAPIGlobal<DrvAPIPointer<T>, MEMTYPE>
{
public:
    /**
     * @brief constructor
     */
    DrvAPIGlobal() {
        initOffset();
    }

    // not copyable or movable
    DrvAPIGlobal(const DrvAPIGlobal &other) = delete;
    DrvAPIGlobal(DrvAPIGlobal &&other) = delete;
    DrvAPIGlobal &operator=(const DrvAPIGlobal &other) = delete;
    DrvAPIGlobal &operator=(DrvAPIGlobal &&other) = delete;


    void initOffset() {
        DrvAPISection &section = DrvAPISection::GetSection(MEMTYPE);
        offset_ = section.increaseSizeBy(sizeof(T));
    }

    DrvAPIPointer<DrvAPIPointer<T>> pointer() const {
        DrvAPIAddress base
            = DrvAPISection::GetSection(MEMTYPE)
            .getBase(myPXNId(), myPodId(), myCoreId());
        return DrvAPIPointer<DrvAPIPointer<T>>(base + offset_);
    }

    /**
     * cast operator
     */
    operator DrvAPIPointer<T>() const { return static_cast<DrvAPIPointer<T>>(*pointer()); }

    /**
     * assignment operator
     */
    DrvAPIGlobal& operator=(const DrvAPIPointer<T> &other) {
        *pointer() = other;
        return *this;
    }

    /**
     * address operator
     */
    DrvAPIPointer<DrvAPIPointer<T>> operator&() const { return pointer(); }

    /**
     * subscript operator
     */
    typename DrvAPIPointer<T>::value_handle
    operator[](size_t idx) const {
        DrvAPIPointer<T> p = *pointer();
        return p[idx];
    }

    uint64_t offset_; //!< offset from the section base
};

template <typename T>
using DrvAPIGlobalL1SP = DrvAPIGlobal<T, DrvAPIMemoryType::DrvAPIMemoryL1SP>;

template <typename T>
using DrvAPIGlobalL2SP = DrvAPIGlobal<T, DrvAPIMemoryType::DrvAPIMemoryL2SP>;

template <typename T>
using DrvAPIGlobalDRAM = DrvAPIGlobal<T, DrvAPIMemoryType::DrvAPIMemoryDRAM>;
}
#endif
