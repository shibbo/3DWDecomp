/**
 * @file Heap.h
 * @brief VFX heap implementation.
 */

#pragma once

#include <nn/types.h>

namespace nn {
namespace vfx {
class Heap {
public:
    virtual ~Heap() {}
    virtual void* Alloc(size_t size, size_t alignment) = 0;
    virtual void Free(void* ptr) = 0;
};

namespace detail {

/**
 * Heap wrapper that forwards to another heap and records how much was allocated through it.
 */
class CalculateAllocatedSizeHeap : public Heap {
public:
    CalculateAllocatedSizeHeap() : m_pHeap(nullptr), m_AllocatedSize(0), m_AllocatedCount(0) {}

    /**
     * Allocates from the wrapped heap.
     * @param size the number of bytes to allocate
     * @param alignment the required alignment
     * @return the allocated memory
     */
    void* Alloc(size_t size, size_t alignment) override {
        void* ptr = m_pHeap->Alloc(size, alignment);
        m_AllocatedSize += size;
        m_AllocatedCount++;
        return ptr;
    }

    /**
     * Returns memory to the wrapped heap.
     * @param ptr the memory to free
     */
    void Free(void* ptr) override {
        m_AllocatedCount--;
        m_pHeap->Free(ptr);
    }

    void SetHeap(Heap* pHeap) { m_pHeap = pHeap; }
    size_t GetAllocatedSize() const { return m_AllocatedSize; }
    s32 GetAllocatedCount() const { return m_AllocatedCount; }

    Heap* m_pHeap;
    size_t m_AllocatedSize;
    s32 m_AllocatedCount;
};

}  // namespace detail
}  // namespace vfx
}  // namespace nn
