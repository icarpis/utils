#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <cstdint>
#include <string>
#include <cstdlib>
#include <mutex>


typedef struct
{
    uint32_t   front;
    uint32_t   rear;
    uint32_t   num_of_items_in_q;
    uint32_t   queue_size_in_bytes;
    uint32_t   free_queue_size_in_bytes;
    std::mutex mtx;
    void* mem_address;
} queue_handler_s;


class dynamic_safe_queue final
{
public:
    static dynamic_safe_queue* get_instance()
    {
        static dynamic_safe_queue singleton;
        return &singleton;
    }

    bool init_queue(queue_handler_s* queue_handler, uint32_t queue_size_in_bytes)
    {
        if (!queue_handler || (0 == queue_size_in_bytes))
        {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(queue_handler->mtx);

            queue_handler->mem_address = malloc(queue_size_in_bytes);
            if (!queue_handler->mem_address)
            {
                return false;
            }

            queue_handler->queue_size_in_bytes = queue_size_in_bytes;
            queue_handler->free_queue_size_in_bytes = queue_size_in_bytes;
            queue_handler->front = 0;
            queue_handler->num_of_items_in_q = 0;
            queue_handler->rear = 0;
        }

        return true;
    }

    void destroy_queue(queue_handler_s* queue_handler)
    {
        std::lock_guard<std::mutex> lock(queue_handler->mtx);

        if (queue_handler->mem_address)
        {
            free(queue_handler->mem_address);
        }
    }

    bool enqueue(queue_handler_s* queue_handler, const void* item, uint32_t item_size_in_bytes)
    {
        if (!queue_handler || !item)
        {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(queue_handler->mtx);

            if (!queue_handler->mem_address)
            {
                return false;
            }

            uint32_t total_item_size = sizeof(item_header_s) + item_size_in_bytes;
            if (total_item_size > queue_handler->free_queue_size_in_bytes)
            {
                return false;
            }

            item_header_s* item_header = (item_header_s*)((uint8_t*)(queue_handler->mem_address) + queue_handler->rear);

            uint32_t continuous_mem_block_size = queue_handler->queue_size_in_bytes - queue_handler->rear;
            if (total_item_size > continuous_mem_block_size)
            {
                if (continuous_mem_block_size >= sizeof(item_header_s))
                {
                    item_header->item_size = item_size_in_bytes;
                    continuous_mem_block_size = queue_handler->queue_size_in_bytes - (queue_handler->rear + sizeof(item_header_s));

                    if (continuous_mem_block_size == 0)
                    {
                        memcpy((uint8_t*)(queue_handler->mem_address), item, item_size_in_bytes);
                    }
                    else
                    {
                        memcpy(item_header->item_ptr, item, continuous_mem_block_size);
                        memcpy((uint8_t*)(queue_handler->mem_address),
                            (const uint8_t*)item + continuous_mem_block_size,
                            item_size_in_bytes - continuous_mem_block_size);
                    }
                }
                else
                {
                    item_header_s temp_item_header;
                    temp_item_header.item_size = item_size_in_bytes;

                    memcpy(item_header, &temp_item_header, continuous_mem_block_size);
                    memcpy(queue_handler->mem_address,
                        (const uint8_t*)&temp_item_header + continuous_mem_block_size,
                        sizeof(item_header_s) - continuous_mem_block_size);
                    memcpy((uint8_t*)(queue_handler->mem_address) + sizeof(item_header_s) - continuous_mem_block_size,
                        item, item_size_in_bytes);
                }
            }
            else
            {
                item_header->item_size = item_size_in_bytes;
                memcpy(item_header->item_ptr, item, item_size_in_bytes);
            }

            queue_handler->rear = (queue_handler->rear + total_item_size) % queue_handler->queue_size_in_bytes;
            queue_handler->free_queue_size_in_bytes -= total_item_size;
            ++(queue_handler->num_of_items_in_q);
        }

        return true;
    }

    bool dequeue(queue_handler_s* queue_handler, void* item,
                 uint32_t item_size_in_bytes, uint32_t* actual_item_size_in_bytes)
    {
        if (!queue_handler || !item || !actual_item_size_in_bytes)
        {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(queue_handler->mtx);

            if (!_peek(queue_handler, item, item_size_in_bytes, actual_item_size_in_bytes))
            {
                return false;
            }

            uint32_t total_item_size = sizeof(item_header_s) + (*actual_item_size_in_bytes);
            queue_handler->front = (queue_handler->front + total_item_size) % queue_handler->queue_size_in_bytes;
            queue_handler->free_queue_size_in_bytes += total_item_size;
            --(queue_handler->num_of_items_in_q);
        }

        return true;
    }

    bool peek(queue_handler_s* queue_handler, void* item,
              uint32_t item_size_in_bytes, uint32_t* actual_item_size_in_bytes)
    {
        if (!queue_handler || !item || !actual_item_size_in_bytes)
        {
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(queue_handler->mtx);
            return _peek(queue_handler, item, item_size_in_bytes, actual_item_size_in_bytes);
        }
    }

    uint32_t size(queue_handler_s* queue_handler)
    {
        if (!queue_handler)
        {
            return 0;
        }

        {
            std::lock_guard<std::mutex> lock(queue_handler->mtx);
            return queue_handler->num_of_items_in_q;
        }
    }


private:

    dynamic_safe_queue() { }
    dynamic_safe_queue(const dynamic_safe_queue&);
    dynamic_safe_queue(const dynamic_safe_queue&&);

    typedef struct
    {
        uint32_t   item_size;

        uint8_t item_ptr[0];
    } item_header_s;


    bool _peek(queue_handler_s* queue_handler, void* item,
               uint32_t item_size_in_bytes, uint32_t* actual_item_size_in_bytes)
    {
        if (!queue_handler->mem_address)
        {
            return false;
        }

        item_header_s* item_header = (item_header_s*)((uint8_t*)(queue_handler->mem_address) + queue_handler->front);

        uint32_t continuous_mem_block_size = queue_handler->queue_size_in_bytes - queue_handler->front;
        if (continuous_mem_block_size >= sizeof(item_header_s))
        {
            if (item_header->item_size > item_size_in_bytes)
            {
                return false;
            }

            *actual_item_size_in_bytes = item_header->item_size;

            continuous_mem_block_size = queue_handler->queue_size_in_bytes - (queue_handler->front + sizeof(item_header_s));
            if (continuous_mem_block_size == 0)
            {
                memcpy(item, queue_handler->mem_address, item_header->item_size);
            }
            else if (item_header->item_size <= continuous_mem_block_size)
            {
                memcpy(item, item_header->item_ptr, item_header->item_size);
            }
            else
            {
                memcpy(item, item_header->item_ptr, continuous_mem_block_size);
                memcpy((uint8_t*)item + continuous_mem_block_size,
                    queue_handler->mem_address,
                    item_header->item_size - continuous_mem_block_size);
            }
        }
        else
        {
            item_header_s temp_item_header;
            memcpy(&temp_item_header, item_header, continuous_mem_block_size);
            memcpy((uint8_t*)&temp_item_header + continuous_mem_block_size,
                queue_handler->mem_address,
                sizeof(item_header_s) - continuous_mem_block_size);

            if (temp_item_header.item_size > item_size_in_bytes)
            {
                return false;
            }

            *actual_item_size_in_bytes = temp_item_header.item_size;

            memcpy(item,
                (uint8_t*)(queue_handler->mem_address) + sizeof(item_header_s) - continuous_mem_block_size,
                temp_item_header.item_size);
        }

        return true;
    }
};
#endif // SAFE_QUEUE_H
