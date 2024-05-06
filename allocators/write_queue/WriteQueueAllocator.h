#ifndef WRITEQUEUECPP_WRITEQUEUEALLOCATOR_H
#define WRITEQUEUECPP_WRITEQUEUEALLOCATOR_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>
#include <sys/mman.h>

extern "C" {
    #include "peartree.h"
}

template<class T>
struct WriteQueueAllocator
{
    [[maybe_unused]] typedef T value_type;

    PearTree tree = (PearTree){nullptr};

    explicit WriteQueueAllocator(size_t heap_size);

    template<class U>
    constexpr explicit WriteQueueAllocator(const WriteQueueAllocator <U>&) noexcept;

    [[maybe_unused]] T* allocate(std::size_t n);

    [[maybe_unused]] void deallocate(T* p, std::size_t n) noexcept;
};

template<class T, class U>
bool operator==(const WriteQueueAllocator <T>&, const WriteQueueAllocator <U>&);

template<class T, class U>
bool operator!=(const WriteQueueAllocator <T>&, const WriteQueueAllocator <U>&);


#endif //WRITEQUEUECPP_WRITEQUEUEALLOCATOR_H
