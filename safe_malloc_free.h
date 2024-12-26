#ifndef SAFE_MALLOC_FREE_H
#define SAFE_MALLOC_FREE_H


#include <unordered_map>
#include <memory>

static std::unordered_map<void*, std::shared_ptr<void>> shared_ptr_per_address;

std::weak_ptr<void> safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        return {};
    }

    // Create a shared_ptr that uses a custom deleter for safe cleanup
    auto sptr = std::shared_ptr<void>(ptr, [](void* p) {
        free(p);
        });

    // Store in the map
    shared_ptr_per_address[ptr] = sptr;

    return sptr;
}

void safe_free(std::weak_ptr<void> wptr)
{
    if (std::shared_ptr<void> sptr = wptr.lock())
    {
        void* ptr = sptr.get();
        shared_ptr_per_address.erase(ptr);
    }
}

#endif
