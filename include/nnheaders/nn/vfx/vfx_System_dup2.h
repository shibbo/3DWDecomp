/**
 * @file vfx_System_dup2.h
 * @brief VFX dynamic heap and the sort helpers used by the VFX system.
 */

#pragma once

#include <nn/types.h>
#include <nn/vfx/Heap.h>
#include <nn/vfx/System.h>

namespace nn {
namespace vfx {

namespace detail {
void* AllocFromDynamicHeap(size_t size, size_t alignment, size_t allocUnit);
void FreeFromDynamicHeap(void* ptr, bool isImmediate);
}  // namespace detail

/**
 * Heap that allocates from the VFX dynamic heap and keeps allocation statistics.
 */
class DynamicHeap : public Heap {
public:
    DynamicHeap() : m_AllocatedSize(0), m_AllocatedCount(0) {}
    ~DynamicHeap() override;
    void* Alloc(size_t size, size_t alignment) override;
    void Free(void* ptr) override;

    size_t GetAllocatedSize() const { return m_AllocatedSize; }
    s32 GetAllocatedCount() const { return m_AllocatedCount; }

    /**
     * Returns memory of a known size to the VFX dynamic heap and removes it from the statistics.
     * @param ptr the memory to free
     * @param size the size the memory was allocated with
     */
    void Free(void* ptr, size_t size) {
        m_AllocatedCount--;
        m_AllocatedSize -= (size + 0xFF) & ~static_cast<size_t>(0xFF);
        detail::FreeFromDynamicHeap(ptr, true);
    }

    size_t m_AllocatedSize;
    s32 m_AllocatedCount;
};

/**
 * Element sorted when ordering emitter sets for drawing.
 */
struct System::SortEmitterSetData {
    EmitterSet* pEmitterSet;
    u32 z;
    u32 index;
};

namespace detail {

/**
 * Element sorted when ordering particles for drawing.
 */
struct SortData {
    f32 z;
    s32 index;
};

/**
 * Orders elements by descending unsigned key.
 */
template <typename T>
struct SortCompareLessUInt {
    bool operator()(const T& rLhs, const T& rRhs) const { return rLhs.z > rRhs.z; }
};

/**
 * Orders elements by ascending key, keeping equal keys in index order.
 */
template <typename T>
struct SortCompareGreaterIndexStable {
    bool operator()(const T& rLhs, const T& rRhs) const {
        if (rLhs.z > rRhs.z) {
            return false;
        }

        if (rLhs.z < rRhs.z) {
            return true;
        }

        return rLhs.index < rRhs.index;
    }
};

/**
 * Orders elements by descending key, keeping equal keys in index order.
 */
template <typename T>
struct SortCompareLessIndexStable {
    bool operator()(const T& rLhs, const T& rRhs) const {
        if (rLhs.z < rRhs.z) {
            return false;
        }

        if (rLhs.z > rRhs.z) {
            return true;
        }

        return rLhs.index < rRhs.index;
    }
};

/**
 * Orders elements by view depth, back to front.
 */
template <typename T>
struct SortCompareViewInvZ {
    bool operator()(const T& rLhs, const T& rRhs) const {
        if (rLhs.z < 0.0f && rRhs.z < 0.0f) {
            return rLhs.z > rRhs.z;
        }

        return rLhs.z < rRhs.z;
    }
};

/**
 * Orders elements by view depth, front to back.
 */
template <typename T>
struct SortCompareViewZ {
    bool operator()(const T& rLhs, const T& rRhs) const {
        if (rLhs.z < 0.0f && rRhs.z < 0.0f) {
            return rLhs.z < rRhs.z;
        }

        return rLhs.z > rRhs.z;
    }
};

}  // namespace detail
}  // namespace vfx
}  // namespace nn
