#pragma once

//we need to use arena allocation instead of dynamic allocation for cache friendliness
//and we need it because of the pointer definitions of nodes to avoid circular defs

#include <cstddef>
#include <memory>
#include <utility>

class ArenaAllocator {
public:
    inline ArenaAllocator(size_t bytes) 
        : m_size(bytes)
    {
        m_buffer = static_cast<std::byte*>(malloc(m_size));
        m_offset = m_buffer;
    }

    template<typename T>
    inline T* alloc() {
        void* offset = m_offset;
        m_offset += sizeof(T);
        return static_cast<T*>(offset);
    }

    inline ArenaAllocator(const ArenaAllocator& other) = delete; //copy constructor

    inline ArenaAllocator operator=(const ArenaAllocator& other) = delete;

    inline ~ArenaAllocator() {
        free(m_buffer);         //desctructor
    }


private:
    size_t m_size;
    std::byte* m_buffer;
    std::byte* m_offset;
};