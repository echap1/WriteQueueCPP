#include "MallocAllocator.h"

template<class T>
template<class U>
constexpr MallocAllocator<T>::MallocAllocator(const MallocAllocator <U>&) noexcept {}

template<class T>
[[maybe_unused]] T* MallocAllocator<T>::allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
        throw std::bad_array_new_length();

    if (auto p = static_cast<T*>(std::malloc(n * sizeof(T))))
    {
        report(p, n);
        return p;
    }

    throw std::bad_alloc();
}

template<class T>
[[maybe_unused]] void MallocAllocator<T>::deallocate(T* p, std::size_t n) noexcept {
    report(p, n, 0);
    std::free(p);
}

template<class T>
void MallocAllocator<T>::report(T* p, std::size_t n, bool alloc) const
{
    std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n
              << " bytes at " << std::hex << std::showbase
              << reinterpret_cast<void*>(p) << std::dec << '\n';
}

template<class T, class U>
bool operator==(const MallocAllocator <T>&, const MallocAllocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const MallocAllocator <T>&, const MallocAllocator <U>&) { return false; }