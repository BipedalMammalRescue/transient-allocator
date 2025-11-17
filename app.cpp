#include "transient_allocator.h"

int main()
{
    TransientAllocator allocator(1024);

    // allocate 5 * 128 byte blocks
    void* buffers[5];
    for (int i = 0; i < 5; i++)
    {
        buffers[i] = allocator.Allocate(128);
    }

    // deallocate #1, 3
    allocator.Free(buffers[1]);
    allocator.Free(buffers[3]);

    // deallocate #4, should trigger a rightward 
    allocator.Free(buffers[4]);

    // deallocate #0, should trigger a leftward
    allocator.Free(buffers[0]);

    // deallocate #2, should trigger both
    allocator.Free(buffers[2]);
}