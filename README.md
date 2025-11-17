# Transient Allocator

A light-weight memory allocator that allocates small buffers on a pre-allocated buffer range.
Allocations are allocated on either the right or left available block, and deallocation only creates available blocks when the freed block connect into one of the available block.