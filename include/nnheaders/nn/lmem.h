/**
 * @file lmem.h
 * @brief Lightweight memory heaps (minimal declaration).
 */

#pragma once

#include <nn/types.h>

namespace nn {
namespace lmem {

namespace detail {
struct HeapHead;
}  // namespace detail

typedef detail::HeapHead* HeapHandle;

HeapHandle CreateFrameHeap(void* pAddress, size_t size, int option);
void DestroyFrameHeap(HeapHandle handle);
void* AllocateFromFrameHeap(HeapHandle handle, size_t size, int alignment);

}  // namespace lmem
}  // namespace nn
