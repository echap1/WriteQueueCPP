#ifndef WRITEQUEUECPP_MALLOCALLOCATOR_H
#define WRITEQUEUECPP_MALLOCALLOCATOR_H

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>

template<class T>
struct MallocAllocator
{
    [[maybe_unused]] typedef T value_type;

    MallocAllocator() = default;

    template<class U>
    constexpr explicit MallocAllocator(const MallocAllocator <U>&) noexcept;

    [[maybe_unused]] T* allocate(std::size_t n);

    [[maybe_unused]] void deallocate(T* p, std::size_t n) noexcept;
private:
    void report(T* p, std::size_t n, bool alloc = true) const;
};

template<class T, class U>
bool operator==(const MallocAllocator <T>&, const MallocAllocator <U>&);

template<class T, class U>
bool operator!=(const MallocAllocator <T>&, const MallocAllocator <U>&);


#endif //WRITEQUEUECPP_MALLOCALLOCATOR_H
