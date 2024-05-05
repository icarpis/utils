#include "custom_allocator.h"
#include "mem_pool.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <mutex>
#include "peterson's_algo_for_n_process.h"


#define SIZE (1024*1024)
uint8_t lmem[SIZE] = { 0 };



std::mutex mtx;

void cpu0()
{
    enter_critical_section(0);
    int x = 1;
    x++;

    // critical section
    {
        std::cout << "P0\n";
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        {
            if (!lock.try_lock())
            {
                std::cout << "Race Condition!!!!!!!!!!";
            }
        }
    }

    leave_critical_section(0);
}

void cpu1()
{
    enter_critical_section(1);
    int x = 1;
    x++;

    // critical section
    {
        std::cout << "P1\n";
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        {
            if (!lock.try_lock())
            {
                std::cout << "Race Condition!!!!!!!!!!";
            }
        }
    }

    leave_critical_section(1);
}

void cpu2()
{
    enter_critical_section(2);
    int x = 1;
    x++;

    // critical section
    {
        std::cout << "P2\n";
        std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
        {
            if (!lock.try_lock())
            {
                std::cout << "Race Condition!!!!!!!!!!";
            }
        }
    }

    leave_critical_section(2);
}


int main()
{
    // memory_pool
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


    // Peterson's algo for n process
    std::thread t11([]() { while (1) { cpu0(); } });
    std::thread t22([]() { while (1) { cpu1(); } });
    std::thread t33([]() { while (1) { cpu2(); } });
    
    t11.join();
    
    return 0;
}
