// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington
#include <stdlib.h>
#include <pandohammer/atomic.h>
#include <pandohammer/cpuinfo.h>
#include <pandohammer/mmio.h>

#define NOBODY (-1)
#define ME                                                      \
    (myThreadId() + myCoreId()*myCoreThreads())

struct __lock {
    int64_t owner;
};

#define LOCK_INITIALIZER { NOBODY }

struct __lock __lock___sinit_recursive_mutex = LOCK_INITIALIZER;
struct __lock __lock___sfp_recursive_mutex = LOCK_INITIALIZER;
struct __lock __lock___atexit_recursive_mutex = LOCK_INITIALIZER;
struct __lock __lock___at_quick_exit_mutex = LOCK_INITIALIZER;
struct __lock __lock___malloc_recursive_mutex = LOCK_INITIALIZER;
struct __lock __lock___env_recursive_mutex = LOCK_INITIALIZER;
struct __lock __lock___tz_mutex = LOCK_INITIALIZER;
struct __lock __lock___dd_hash_mutex = LOCK_INITIALIZER;
struct __lock __lock___arc4random_mutex = LOCK_INITIALIZER;

void
__retarget_lock_init (_LOCK_T *lock)
{
    __retarget_lock_init_recursive(lock);
}

void
__retarget_lock_init_recursive(_LOCK_T *lock)
{
    *lock = malloc(sizeof(struct __lock));
    (*lock)->owner = NOBODY;
}

void
__retarget_lock_close(_LOCK_T lock)
{
    __retarget_lock_close_recursive(lock);
}

void
__retarget_lock_close_recursive(_LOCK_T lock)
{

}

void
__retarget_lock_acquire (_LOCK_T lock)
{
    __retarget_lock_acquire_recursive(lock);
}

void
__retarget_lock_acquire_recursive (_LOCK_T lock)
{
    if (lock->owner == ME) {
        return;
    }
    while (atomic_compare_and_swap_i64(&lock->owner, NOBODY, ME) != NOBODY);
}

int
__retarget_lock_try_acquire(_LOCK_T lock)
{
    return __retarget_lock_try_acquire_recursive(lock);
}

int
__retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (lock->owner == ME) {
        return 1;
    }
    return atomic_compare_and_swap_i64(&lock->owner, NOBODY, ME) == NOBODY;
}

void
__retarget_lock_release (_LOCK_T lock)
{
    __retarget_lock_release_recursive(lock);
}

void
__retarget_lock_release_recursive (_LOCK_T lock)
{
    if (lock->owner != ME) {
        return;
    }
    lock->owner = NOBODY;
}
