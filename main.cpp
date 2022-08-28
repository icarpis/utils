#include "custom_allocator.h"
#include "mem_pool.h"



#define SIZE (1024*1024)
uint8_t lmem[SIZE] = { 0 };


int main()
{
    while (1)
    {
        {
            memory_pool<> mp(SIZE);
            void* ptr = mp.malloc(111, 1024);
            mp.free(ptr);
        }

        {
            memory_pool<custom_allocator<uint8_t>> mp(SIZE);
            void* ptr = mp.malloc(SIZE / 4);
            mp.free(ptr);
        }

        {
            memory_pool<custom_allocator<uint8_t>> mp(SIZE);
            void* ptr = mp.malloc(SIZE);
            mp.free(ptr);
        }

        {
            memory_pool<custom_allocator<uint8_t>> mp(SIZE);
            void* ptr = mp.malloc(10, SIZE);
            mp.free(ptr);
        }

        {
            memory_pool<custom_allocator<uint8_t>> mp(SIZE);
            void* ptr = mp.malloc(1024, 128);
            mp.free(ptr);
        }
    }
    return 0;
}