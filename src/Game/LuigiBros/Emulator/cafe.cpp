#include "LuigiBros/Emulator/cafe.hpp"
#include "Library/Controller/InputFunction.hpp"
#include <cstring>

namespace nn { namespace os { void FlushDataCache(const void* pMemory, unsigned long size); } }

/**
 * @brief Platform stub for GPU timing.
 * @param pCycle Unused compatibility argument.
 */
void GX2SampleTopGPUCycle(unsigned long* pCycle) {}

/**
 * @brief Platform stub for GPU timing.
 * @param pCycle Unused compatibility argument.
 */
void GX2SampleBottomGPUCycle(unsigned long* pCycle) {}

/**
 * @brief Platform stub for system time.
 * @return Always zero.
 */
unsigned long OSGetSystemTime() {
    return 0;
}

/**
 * @brief Converts nanosecond ticks to microseconds.
 * @param ticks Tick count.
 * @return Converted duration.
 */
unsigned long OSTicksToMicroseconds(unsigned long ticks) {
    return ticks / 1000;
}

/**
 * @brief Converts nanosecond ticks to milliseconds.
 * @param ticks Tick count.
 * @return Converted duration.
 */
unsigned long OSTicksToMilliseconds(unsigned long ticks) {
    return ticks / 1000000;
}

/**
 * @brief Platform stub for converting milliseconds to ticks.
 * @param milliseconds Unused compatibility argument.
 * @return Always zero.
 */
unsigned long OSMillisecondsToTicks(unsigned long long milliseconds) {
    return 0;
}

/**
 * @brief Platform stub for converting ticks to seconds.
 * @param ticks Unused compatibility argument.
 * @return Always zero.
 */
unsigned long OSTicksToSeconds(unsigned long ticks) {
    return 0;
}

/**
 * @brief Platform stub for reading the clock.
 * @return Always zero.
 */
unsigned long OSGetTick() {
    return 0;
}

/**
 * @brief Platform stub for reading the clock.
 * @return Always zero.
 */
unsigned long OSGetTime() {
    return 0;
}

/**
 * @brief Computes elapsed ticks.
 * @param start Starting tick.
 * @param end Ending tick.
 * @return Elapsed tick count.
 */
unsigned long OSDiffTick(unsigned long start, unsigned long end) {
    return end - start;
}

/**
 * @brief Platform stub for heap information.
 * @param pHeap Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetHeapType(nn::lmem::detail::HeapHead* pHeap) {
    return 0;
}

/**
 * @brief Platform stub for heap information.
 * @param pHeap Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetHeapEndAddress(nn::lmem::detail::HeapHead* pHeap) {
    return 0;
}

/**
 * @brief Platform stub for heap information.
 * @param pHeap Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetHeapStartAddress(nn::lmem::detail::HeapHead* pHeap) {
    return 0;
}

/**
 * @brief Platform stub for heap information.
 * @param pHeap Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetTotalFreeSizeForExpHeap(nn::lmem::detail::HeapHead* pHeap) {
    return 0;
}

/**
 * @brief Platform stub for heap information.
 * @param pHeap Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetTrackingLeftInBlockHeap(nn::lmem::detail::HeapHead* pHeap) {
    return 0;
}

/**
 * @brief Platform stub for allocatable heap size.
 * @param pHeap Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetAllocatableSizeForExpHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment) {
    return 0;
}

/**
 * @brief Platform stub for allocatable heap size.
 * @param pHeap Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetAllocatableSizeForFrmHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment) {
    return 0;
}

/**
 * @brief Platform stub for allocatable heap size.
 * @param pHeap Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMGetAllocatableSizeForBlockHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment) {
    return 0;
}

/**
 * @brief Platform stub for heap allocation.
 * @param pHeap Unused compatibility argument.
 * @param size Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always nullptr.
 */
void* MEMAllocFromExpHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment) {
    return nullptr;
}

/**
 * @brief Platform stub for heap allocation.
 * @param pHeap Unused compatibility argument.
 * @param size Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always nullptr.
 */
void* MEMAllocFromFrmHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment) {
    return nullptr;
}

/**
 * @brief Platform stub for heap allocation.
 * @param pHeap Unused compatibility argument.
 * @param size Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always nullptr.
 */
void* MEMAllocFromBlockHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment) {
    return nullptr;
}

/**
 * @brief Platform stub for releasing heap memory.
 * @param pHeap Unused compatibility argument.
 * @param pMemory Unused compatibility argument.
 */
void MEMFreeToExpHeap(nn::lmem::detail::HeapHead* pHeap, void* pMemory) {}

/**
 * @brief Platform stub for releasing heap memory.
 * @param pHeap Unused compatibility argument.
 * @param pMemory Unused compatibility argument.
 */
void MEMFreeToBlockHeap(nn::lmem::detail::HeapHead* pHeap, void* pMemory) {}

/**
 * @brief Platform stub for releasing frame heap memory.
 * @param pHeap Unused compatibility argument.
 * @param mode Unused compatibility argument.
 */
void MEMFreeToFrmHeap(nn::lmem::detail::HeapHead* pHeap, unsigned int mode) {}

/**
 * @brief Platform stub for block heap tracking.
 * @param pHeap Unused compatibility argument.
 * @param pTracking Unused compatibility argument.
 * @param size Unused compatibility argument.
 * @return Always zero.
 */
unsigned int MEMAddBlockHeapTracking(nn::lmem::detail::HeapHead* pHeap, unsigned char* pTracking, unsigned int size) {
    return 0;
}

/**
 * @brief Platform stub for the base heap handle.
 * @param arena Unused compatibility argument.
 * @return Always nullptr.
 */
nn::lmem::detail::HeapHead* MEMGetBaseHeapHandle(MEMArena arena) {
    return nullptr;
}

/**
 * @brief Platform compatibility stub.
 */
void GX2Shutdown() {}

/**
 * @brief Platform compatibility stub.
 */
void ProcUIClearCallbacks() {}

/**
 * @brief Returns the fixed platform region identifier.
 * @return Fixed region identifier 1.
 */
int getPlatformRegion() {
    return 1;
}

/**
 * @brief Platform stub for screen capture permission.
 * @param enabled Unused compatibility argument.
 */
void OSSetScreenCapturePermission(bool enabled) {}

/**
 * @brief Platform stub for sleeping.
 * @param microseconds Unused compatibility argument.
 */
void OSSleepMicroseconds(unsigned int microseconds) {}

/**
 * @brief Platform stub for the legacy allocator.
 * @param size Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always nullptr.
 */
void* VESSEL_malloc(unsigned int size, unsigned int alignment) {
    return nullptr;
}

/**
 * @brief Platform stub for the legacy allocator.
 * @param size Unused compatibility argument.
 * @param alignment Unused compatibility argument.
 * @return Always nullptr.
 */
void* VESSEL_MALLOCEX(unsigned int size, unsigned int alignment) {
    return nullptr;
}

/**
 * @brief Platform stub for the legacy allocator.
 * @param pMemory Unused compatibility argument.
 */
void VESSEL_FREE(void* pMemory) {}

/**
 * @brief Platform compatibility stub.
 */
void OSEnableForegroundExit() {}

/**
 * @brief Platform compatibility stub.
 */
void OSSavesDone_ReadyToRelease() {}

/**
 * @brief Platform compatibility stub.
 */
void ProcUIDrawDoneRelease() {}

/**
 * @brief Returns the platform placeholder value.
 * @return Always zero.
 */
unsigned int OSGetCoreId() {
    return 0;
}

/**
 * @brief Returns the platform placeholder value.
 * @return Always zero.
 */
unsigned int OSGetStackPointer() {
    return 0;
}

/**
 * @brief Copies a block of memory.
 * @param pDestination Destination buffer.
 * @param pSource Source buffer.
 * @param size Number of bytes to copy.
 * @param flush Unused cache flush flag.
 * @return Destination buffer.
 */
void* OSBlockMove(unsigned int* pDestination, unsigned int* pSource, unsigned int size, bool flush) {
    return std::memcpy(pDestination, pSource, size);
}

/**
 * @brief Flushes a range from the data cache.
 * @param pMemory Start of the memory range.
 * @param size Number of bytes to flush.
 */
void DCFlushRange(void* pMemory, unsigned int size) {
    nn::os::FlushDataCache(pMemory, size);
}

/**
 * @brief Reverses the byte order of a word.
 * @param value Word to byte-swap.
 * @return Word with reversed byte order.
 */
unsigned int inverseBits(unsigned int value) {
    return __builtin_bswap32(value);
}

/**
 * @brief Platform stub for releasing default heap memory.
 * @param pMemory Unused compatibility argument.
 */
void MEMFreeToDefaultHeap(void* pMemory) {}

/**
 * @brief Populates legacy controller status with the main controller stick positions.
 * @param channel Index into the status array.
 * @param pStatus Output status array.
 * @param count Unused requested sample count.
 * @param pError Output error code, always zero.
 * @return Zero after updating the status.
 */
int VPADRead(int channel, VPADStatus* pStatus, unsigned int count, int* pError) {
    VPADStatus* pResult = &pStatus[channel];
    *pError = 0;
    std::memset(pResult, 0, sizeof(VPADStatus));
    const sead::Vector2f& rLeft = al::getLeftStick(al::getMainControllerPort());
    const sead::Vector2f& rRight = al::getRightStick(al::getMainControllerPort());
    pResult->leftX = rLeft.x;
    pResult->leftY = rLeft.y;
    pResult->rightX = rRight.x;
    pResult->rightY = rRight.y;
    pResult->error = 0xff;
    return 0;
}

/**
 * @brief Platform stub for touch calibration.
 * @param channel Unused compatibility argument.
 * @param pOutput Unused compatibility argument.
 * @param pInput Unused compatibility argument.
 */
void VPADGetTPCalibratedPoint(int channel, VPADTPData* pOutput, const VPADTPData* pInput) {}

/**
 * @brief Platform stub for legacy controller input.
 * @param pStatus Unused compatibility argument.
 */
void PADRead(PADStatus* pStatus) {}
