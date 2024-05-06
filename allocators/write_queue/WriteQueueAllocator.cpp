#include "WriteQueueAllocator.h"

template<class T>
WriteQueueAllocator<T>::WriteQueueAllocator(size_t heap_size) {
    std::cout << "[write_queue] initialized tree with heap size " << heap_size << std::endl;
    void* base = mmap(nullptr, heap_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
    assert(sizeof(SignPost) <= MINIMUM);
    init(&tree, base, (long) heap_size);
}

template<class T>
template<class U>
constexpr WriteQueueAllocator<T>::WriteQueueAllocator(const WriteQueueAllocator <U>&) noexcept {}

template<class T>
[[maybe_unused]] T* WriteQueueAllocator<T>::allocate(std::size_t n) {
    void *p = take(&tree, (long) n * sizeof(T));
    std::cout << "[write_queue] allocated " << n * sizeof(T) << " bytes at " << p << std::endl;
    return (int *) p;
}

template<class T>
[[maybe_unused]] void WriteQueueAllocator<T>::deallocate(T* p, std::size_t n) noexcept {
    std::cout << "[write_queue] freed " << n * sizeof(T) << " bytes at " << p << std::endl;
    give(&tree, p);
}

template<class T, class U>
bool operator==(const WriteQueueAllocator <T>&, const WriteQueueAllocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const WriteQueueAllocator <T>&, const WriteQueueAllocator <U>&) { return false; }