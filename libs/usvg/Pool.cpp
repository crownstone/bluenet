

#include "Pool.h"

void* Pool::allocate(size_t sz) {
    if (sz > size) {
        NRF51_CRASH("Too big.");
    }
    if (!free_list ) {
        NRF51_CRASH("Out of memory.");
    }
    void** ret = free_list;
    free_list = (void**)*ret;
    if (free_list && ((uint8_t*)free_list < buffer || (uint8_t*)free_list > buffer + (size*count))) {
        NRF51_CRASH("Corrupt free list.");
    }
    *ret = 0;
    available--;
    return ret;
}

void Pool::release(size_t sz, void* mem) {
    if (sz > size) {
        NRF51_CRASH("Too big.");
    }
    if ((uint8_t*)mem < buffer || (uint8_t*)mem > buffer + (size*count)) {
        NRF51_CRASH("Not a valid pooled memory block.");
    }
    void** m = (void**) mem;
    *m = (void*)free_list;
    free_list = m;
    available++;
}

PoolRegistry pool_registry;

