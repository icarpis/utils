#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <cstdint>
#include <memory>

#ifdef __EXCEPTIONS
#include <exception>
#endif
#include <mutex>


template<typename _Alloc = std::allocator<uint8_t>>
class memory_pool
{
public:
    memory_pool(void* mem_pool, size_t mem_pool_size)
        : m_free_list(nullptr),
          m_mem_pool_size(0),
          m_free_mem(false)
    {
        if (!mem_pool || (mem_pool_size <= sizeof(block_s)) || (0 == mem_pool_size))
        {
#ifdef __EXCEPTIONS
            throw(std::bad_alloc());
#else
            return;
#endif
        }

        init(mem_pool, mem_pool_size);
    }

    explicit memory_pool(size_t mem_pool_size)
        : m_free_list(nullptr),
          m_mem_pool_size(0),
          m_free_mem(false)
    {
        void* mem_pool = m_allocator.allocate(mem_pool_size);
        if (!mem_pool || (mem_pool_size <= sizeof(block_s)) || (0 == mem_pool_size))
        {
#ifdef __EXCEPTIONS
            throw(std::bad_alloc());
#else
            return;
#endif
        }
        
        m_free_mem = true;
        init(mem_pool, mem_pool_size);
    }

    virtual ~memory_pool()
    {
        if (m_free_mem)
        {
            m_allocator.deallocate((uint8_t*)m_free_list, m_mem_pool_size);
        }

        m_free_list = nullptr;
        m_mem_pool_size = 0;
        m_free_mem = false;
    }

    void* malloc(size_t size, size_t align_val = 0)
    {
        std::lock_guard<std::mutex> lock(m_mtx);

        void* ptr = nullptr;

        if (m_free_list && (size != 0) && is_power_of_two(align_val))
        {
            size_t total_size = size + align_val;
            block_s* curr = m_free_list;

            while ((((curr->size) < total_size) || (!(curr->free))) && (curr->next != nullptr))
            {
                curr = curr->next;
            }

            if ((curr->size) == total_size) // Exact fitting block allocated
            {
                curr->align_offset_placeholder = 0;
                curr->free = 0;
                ptr = (void*)(++curr);
            }
            else if ((curr->size) > (total_size + sizeof(block_s))) // Fitting block allocated with a split
            {
                curr->align_offset_placeholder = 0;
                split(curr, total_size);
                ptr = (void*)(++curr);
            }
            else // No sufficient memory to allocate
            {
                ptr = nullptr;
            }
            
            if (ptr && (align_val != 0))
            {
                ptr = align(ptr, align_val);
            }
        }
        
        return ptr;
    }

    void free(void* ptr)
    {
        std::lock_guard<std::mutex> lock(m_mtx);

        if (m_free_list && ptr)
        {
            if (((void*)m_free_list <= ptr) && (ptr <= (void*)((uint8_t*)m_free_list + m_mem_pool_size)))
            {
                uint64_t* align_offset_placeholder_p = (uint64_t*)((uint8_t*)ptr - sizeof(uint64_t));
                if (((void*)m_free_list <= align_offset_placeholder_p) && (align_offset_placeholder_p <= (void*)((uint8_t*)m_free_list + m_mem_pool_size)))
                {
                    block_s* curr = (block_s*)((uint8_t*)ptr - *align_offset_placeholder_p);
                    --curr;
                    curr->free = 1;
                    merge();
                }
            }
        }
    }

private:
    static_assert(std::is_same<_Alloc::value_type, uint8_t>::value, "allocator type must be uint8_t!");

    /* The structure definition to contain metadata of each block allocated or deallocated */
#pragma pack(push, 1)
    struct block_s {
        size_t   size; /* Carries the size of the block described by it */
        uint32_t free; /* This flag is used to know whether the block described by the metadata structure is free or not */
        block_s* next; /* Points to the next metadata block */
        uint64_t align_offset_placeholder;
    };
#pragma pack(pop)

    block_s* m_free_list;
    size_t m_mem_pool_size;
    std::mutex m_mtx;
    bool m_free_mem;
    _Alloc m_allocator;

    void init(void* mem_pool, size_t mem_pool_size)
    {
        m_mem_pool_size = mem_pool_size;
        m_free_list = (block_s*)mem_pool;
        m_free_list->size = mem_pool_size - sizeof(block_s);
        m_free_list->free = 1;
        m_free_list->next = nullptr;
    }

    void split(block_s* fitting_slot, size_t size)
    {
        block_s* new_block = (block_s*)((uint8_t*)fitting_slot + size + sizeof(block_s));
        new_block->size = (fitting_slot->size) - size - sizeof(block_s);
        new_block->free = 1;
        new_block->next = fitting_slot->next;

        fitting_slot->size = size;
        fitting_slot->free = 0;
        fitting_slot->next = new_block;
    }

    
    /* This is to merge the consecutive free blocks by removing the metadata block in the middle. This will save space. */
    void merge()
    {
        block_s* curr = nullptr;
        while (true)
        {
            curr = m_free_list;
            bool free_consecutive_nodes = false;
            while (curr && (curr->next))
            {
                if ((curr->free) && (curr->next->free))
                {
                    curr->size += (curr->next->size) + sizeof(block_s);
                    curr->next = curr->next->next;
                    free_consecutive_nodes = true;
                }

                curr = curr->next;
            }

            if (!free_consecutive_nodes)
            {
                break;
            }
        }
    }

    void* align(void* ptr, size_t align_val)
    {
        void* aligned_ptr = ptr;
        if (ptr)
        {
            if ((uint64_t)align_val > 0)
            {
                uint64_t left_over = ((uint64_t)ptr) % (uint64_t)align_val;
                if (left_over > 0)
                {
                    aligned_ptr = ((uint8_t*)ptr) + (uint64_t)align_val - left_over;    // align the buffer

                    uint64_t* align_offset_placeholder_p = (uint64_t*)((uint8_t*)aligned_ptr - sizeof(uint64_t));
                    *align_offset_placeholder_p = (uint64_t)align_val - left_over;
                }
                else
                {
                    aligned_ptr = ptr;  // original allocation is already aligned
                }
            }
            else
            {
                aligned_ptr = ptr;      // no align is needed
            }
        }
        return aligned_ptr;
    }

    bool is_power_of_two(size_t num) const
    {
        return ((num & (num - 1)) == 0);
    }
};


#endif