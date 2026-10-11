#include <nn/vfx/vfx_System_dup2.h>

#include <algorithm>
#include <nn/nn_SdkAssert.h>

namespace nn {
namespace vfx {

/**
 * Destroys the dynamic heap. Every allocation must have been freed by now.
 */
DynamicHeap::~DynamicHeap() {
    NN_SDK_ASSERT(m_AllocatedCount == 0);
}

/**
 * Allocates memory from the VFX dynamic heap and records it in the statistics.
 * @param size the number of bytes to allocate
 * @param alignment the required alignment
 * @return the allocated memory, or nullptr on failure
 */
void* DynamicHeap::Alloc(size_t size, size_t alignment) {
    void* ptr = detail::AllocFromDynamicHeap(size, alignment, 0x100);

    if (ptr != nullptr) {
        m_AllocatedSize += (size + 0xFF) & ~static_cast<size_t>(0xFF);
        m_AllocatedCount++;
    }

    return ptr;
}

/**
 * Returns memory to the VFX dynamic heap.
 * @param ptr the memory to free
 */
void DynamicHeap::Free(void* ptr) {
    m_AllocatedCount--;
    detail::FreeFromDynamicHeap(ptr, true);
}

}  // namespace vfx
}  // namespace nn

namespace {

typedef nn::vfx::System::SortEmitterSetData SortEmitterSetData;
typedef nn::vfx::detail::SortData SortData;

typedef nn::vfx::detail::SortCompareLessUInt<SortEmitterSetData> CompareLessUInt;
typedef nn::vfx::detail::SortCompareGreaterIndexStable<SortData> CompareGreaterIndexStable;
typedef nn::vfx::detail::SortCompareLessIndexStable<SortData> CompareLessIndexStable;
typedef nn::vfx::detail::SortCompareViewInvZ<SortData> CompareViewInvZ;
typedef nn::vfx::detail::SortCompareViewZ<SortData> CompareViewZ;

}  // namespace

// std::sort instantiations used by nn::vfx::System to order emitter sets and particles.
template void std::__sort<CompareLessUInt&, SortEmitterSetData*>(SortEmitterSetData*,
                                                                 SortEmitterSetData*,
                                                                 CompareLessUInt&);
template void std::__sort<CompareGreaterIndexStable&, SortData*>(SortData*, SortData*,
                                                                 CompareGreaterIndexStable&);
template void std::__sort<CompareLessIndexStable&, SortData*>(SortData*, SortData*,
                                                              CompareLessIndexStable&);
template void std::__sort<CompareViewInvZ&, SortData*>(SortData*, SortData*, CompareViewInvZ&);
template void std::__sort<CompareViewZ&, SortData*>(SortData*, SortData*, CompareViewZ&);
