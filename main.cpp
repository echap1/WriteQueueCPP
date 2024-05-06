#include <vector>

#include "allocators/malloc/MallocAllocator.h"
#include "allocators/malloc/MallocAllocator.cpp"  // I have ZERO idea why this is necessary

#include "allocators/write_queue/WriteQueueAllocator.h"
#include "allocators/write_queue/WriteQueueAllocator.cpp"


int main() {
    WriteQueueAllocator<int> a(1 << 14);

    std::vector<int, WriteQueueAllocator<int>> v(8, a);
    std::vector<int, WriteQueueAllocator<int>> v2(8, a);
    v.push_back(42);

    display(&a.tree, false);
}