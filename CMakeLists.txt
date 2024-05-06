cmake_minimum_required(VERSION 3.27)
project(WriteQueueCPP)

set(CMAKE_CXX_STANDARD 17)

add_executable(WriteQueueCPP main.cpp
        allocators/malloc/MallocAllocator.cpp
        allocators/malloc/MallocAllocator.h
        allocators/write_queue/WriteQueueAllocator.cpp
        allocators/write_queue/WriteQueueAllocator.h
        allocators/write_queue/peartree.c
        allocators/write_queue/peartree.h
)
