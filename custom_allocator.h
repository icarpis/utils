#ifndef CUSTOM_ALLOCATOR_H
#define CUSTOM_ALLOCATOR_H

#include <iostream>
#include <memory>
#include <limits>
#include <cstdint>

#ifdef __EXCEPTIONS
#include <exception>
#endif

template <typename T>
class custom_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    template <typename U> struct rebind
    {
        typedef custom_allocator<U> other;
    };

    custom_allocator()
    {
    }

    pointer allocate(size_type n, const void* hint = 0)
    {
        if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T)))
        {
#ifdef __EXCEPTIONS
            throw(std::length_error());
#else
            return nullptr;
#endif
        }

        pointer x = (pointer)malloc(n * sizeof(T));
        if (x == nullptr)
        {
#ifdef __EXCEPTIONS
            throw(std::bad_alloc());
#else
            return nullptr;
#endif
        }

        return x;
    }

    void deallocate(T* p, std::size_t n)
    {
        free(p);
    }

    template <typename U> custom_allocator(const custom_allocator<U>&)
    {
    }

};

#endif