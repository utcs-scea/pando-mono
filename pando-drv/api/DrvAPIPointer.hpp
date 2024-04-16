// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#ifndef DRV_API_POINTER_H
#define DRV_API_POINTER_H
#include <DrvAPIAddress.hpp>
#include <DrvAPIMemory.hpp>
#include <cstddef>
namespace DrvAPI
{

/**
 * @brief The pointer class
 *
 * @tparam T
 */
template <typename T>
class DrvAPIPointer
{
public:
    typedef DrvAPIPointer<T> pointer_type; //!< pointer type
    typedef T value_type; //!< value type

    /**
     * base constructor
     */
    DrvAPIPointer(DrvAPIAddress vaddr) :vaddr_(vaddr) {}

    /**
     * empty constructor
     */
    DrvAPIPointer() : DrvAPIPointer(DrvAPIAddress()) {}

    /**
     * copy constructor
     */
    DrvAPIPointer(const pointer_type &other) = default;

    /**
     * move constructor
     */
    DrvAPIPointer(pointer_type &&other) = default;

    /**
     * copy assignment
     */
    pointer_type &operator=(const pointer_type &other) = default;

    /**
     * move assignment
     */
    pointer_type &operator=(pointer_type &&other) = default;

    /**
     * destructor
     */
    ~DrvAPIPointer() = default;


    /**
     * cast to another pointer type
     */
    template <typename U>
    operator DrvAPIPointer<U>() const {
        return DrvAPIPointer<U>(vaddr_);
    }

    /**
     * cast operator to DrvAPIAddress
     */
    operator DrvAPIAddress() const {
        return vaddr_;
    }

    /**
     * handle
     */
    class value_handle {
    public:
        /**
         * constructor
         */
        value_handle(const DrvAPIAddress &vaddr) :vaddr_(vaddr) {}
        value_handle() = delete;
        value_handle(const value_handle &other) = delete;
        value_handle(value_handle &&other) = default;
        value_handle &operator=(const value_handle &other) = delete;
        value_handle &operator=(value_handle &&other) = default;
        ~value_handle() = default;

        /**
         * cast operator to type T
         */
        operator value_type() const {
            return DrvAPI::read<T>(vaddr_, PHASE_OTHER);
        }

        /**
         * assignment operator
         */
        value_handle &operator=(const T &value) {
            DrvAPI::write<T>(vaddr_, PHASE_OTHER, value);
            return *this;
        }

        /**
         * address of operator
         */
        pointer_type operator&() {
            return pointer_type(vaddr_);
        }

        DrvAPIAddress vaddr_;
    };

    /**
     * dereference operator
     */
    value_handle operator*() const {
        return value_handle(vaddr_);
    }

    /**
     * post increment operator
     */
    pointer_type operator++(int) {
        pointer_type tmp(*this);
        vaddr_ += sizeof(value_type);
        return tmp;
    }

    /**
     * pre increment operator
     */
    pointer_type &operator++() {
        vaddr_ += sizeof(value_type);
        return *this;
    }

    /**
     * post decrement operator
     */
    pointer_type operator--(int) {
        pointer_type tmp(*this);
        vaddr_ -= sizeof(value_type);
        return tmp;
    }

    /**
     * pre decrement operator
     */
    pointer_type &operator--() {
        vaddr_ -= sizeof(value_type);
        return *this;
    }

    /**
     * add assignment operator
     */
    template <typename integer_type>
    pointer_type &operator+=(integer_type rhs) {
        vaddr_ += rhs*sizeof(value_type);
        return *this;
    }

    /**
     * subtract assignment operator
     */
    template <typename integer_type>
    pointer_type &operator-=(integer_type  rhs) {
        vaddr_ -= rhs*sizeof(value_type);
        return *this;
    }


    /**
     * addition operator
     */
    template <typename integer_type>
    pointer_type operator+(integer_type rhs) const {
        pointer_type tmp(*this);
        tmp += rhs;
        return tmp;
    }

    /**
     * subtraction operator
     */
    template <typename integer_type>
    pointer_type operator-(integer_type rhs) const {
        pointer_type tmp(*this);
        tmp -= rhs;
        return tmp;
    }

    /**
     * index operator
     */
    template <typename integer_type>
    value_handle operator[](integer_type rhs) const {
        return value_handle(vaddr_ + rhs*sizeof(value_type));
    }

    /**
     * warning do not use this operator
     * use the macros instead
     */
    value_type* operator->() {
        return nullptr;
    }

    DrvAPIAddress vaddr_; //!< virtual address
};

#define DRV_API_REF_CLASS_BEGIN(type)                                   \
    class type##_ref {                                                  \
    public:                                                             \
    type##_ref(const DrvAPI::DrvAPIPointer<type>&ptr) : ptr_(ptr) {}    \
    type##_ref(const DrvAPI::DrvAPIAddress &vaddr) : ptr_(vaddr) {}     \
    type##_ref(uint64_t vaddr) : ptr_(vaddr) {}                         \
    type##_ref() = delete;                                              \
    type##_ref(const type##_ref &other) = default;                      \
    type##_ref(type##_ref &&other) = default;                           \
    type##_ref &operator=(const type##_ref &other) = default;           \
    type##_ref &operator=(type##_ref &&other) = default;                \
    ~type##_ref() = default;                                            \
    DrvAPI::DrvAPIPointer<type> operator&() {                           \
        return ptr_;                                                    \
    }                                                                   \
    DrvAPI::DrvAPIPointer<type> ptr_;

#define DRV_API_REF_CLASS_DATA_MEMBER(type, member)                     \
    DrvAPI::DrvAPIPointer<decltype(std::declval<type>().member)>::value_handle \
    member () const {                                                   \
        return *DrvAPI::DrvAPIPointer<decltype(std::declval<type>().member)> \
            (ptr_.vaddr_ + offsetof(type, member));                     \
    }                                                                   \

#define DRV_API_REF_CLASS_END(type)                                     \
    };                                                                  \

}
#endif
