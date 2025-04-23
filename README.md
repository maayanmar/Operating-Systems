# Operating Systems Projects

This repository contains three projects demonstrating key concepts in operating systems:

## 1. User-Level Threads Library
A library for managing user-level threads without kernel involvement. It uses `sigsetjmp` and `siglongjmp` for context switching and supports thread creation, termination, blocking, resuming, and sleeping. The library also includes a scheduler based on virtual timers.

## 2. MapReduce Framework
A multi-threaded implementation of the MapReduce paradigm. It includes:
- **MapReduceClient**: Defines the `map` and `reduce` operations.
- **MapReduceFramework**: Manages job execution, including mapping, shuffling, and reducing phases, using thread synchronization mechanisms like barriers and semaphores.

## 3. Virtual Memory Management
A virtual memory library that simulates paging with a hierarchical page table. It supports:
- Translating virtual addresses to physical addresses.
- Handling page faults with eviction and restoration.
- Efficient memory management using a cyclic distance eviction policy.

Each project showcases advanced operating system concepts and efficient resource management.