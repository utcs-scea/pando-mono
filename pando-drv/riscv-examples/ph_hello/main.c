// SPDX-License-Identifier: MIT
// Copyright (c) 2023 University of Washington

#include <math.h>
#include <string.h>
#include <pandohammer/mmio.h>

#define ARRAY_SIZE(x) \
    (sizeof(x)/sizeof((x)[0]))

__attribute__((section(".dmem")))
char message[] = "Hello, world!\n";

long lock = 1;

#define MIN(a, b) \
    ((a) < (b) ? (a) : (b))

#define LOCKED

int main()
{
#ifdef LOCKED
    long acquired = __atomic_exchange_n(&lock, 0, __ATOMIC_ACQUIRE);
    long backoff = 1;
    long max_backoff = 1 << 0;
    while (!acquired) {
        backoff = MIN(backoff << 1, max_backoff);
        for (volatile long i = 0; i < backoff; i++) {
            __asm__ __volatile__ ("nop");
        }
        acquired = __atomic_exchange_n(&lock, 0, __ATOMIC_ACQUIRE);
    }
#endif
    for (long i = 0; i < strlen(message); i++) {
        ph_print_char(message[i]);
    }
#ifdef LOCKED
    // uncomment this when non-blocking stores are implemented
    //__atomic_thread_fence(__ATOMIC_RELEASE);
    lock = 1;
#endif
    return 0;
}
