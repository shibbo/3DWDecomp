#pragma once
#include <new>
#include <nn/types.h>

namespace nn::atk::detail {
class PoolImpl {
public:
    int CreateImpl(void* memory, size_t size, size_t elementSize);
    int CreateImpl(void* memory, size_t size, size_t elementSize, size_t alignment);
    void DestroyImpl();
    int CountImpl() const;
    void* AllocImpl();
    void FreeImpl(void* memory);
private:
    struct Node { Node* next; };
    Node mRoot;
    void* mMemory;
    size_t mSize;
};
static_assert(sizeof(PoolImpl) == 0x18, "PoolImpl size");

/**
 * @brief Fixed-capacity pool constructing objects of one type in caller-provided memory.
 * @tparam T Pooled object type.
 */
template <typename T>
class InstancePool : public PoolImpl {
public:
    /**
     * @brief Divides memory into object slots.
     * @param pMemory Pool storage.
     * @param size Size of pMemory in bytes.
     * @return Number of slots.
     */
    int Create(void* pMemory, size_t size) { return CreateImpl(pMemory, size, sizeof(T)); }

    /** @brief Forgets the pool storage. */
    void Destroy() { DestroyImpl(); }

    /** @brief Value-initializes an object in a free slot. @return Object, or nullptr when full. */
    T* Alloc() {
        void* pMemory = AllocImpl();
        if (pMemory == nullptr) {
            return nullptr;
        }

        return new (pMemory) T();
    }

    /**
     * @brief Returns a slot to the pool without destroying its object.
     * @param pObject Object obtained from Alloc.
     */
    void Free(T* pObject) { FreeImpl(pObject); }
};
}
