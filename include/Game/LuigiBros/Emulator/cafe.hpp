#pragma once

#include <basis/seadTypes.h>

namespace nn { namespace lmem { namespace detail { struct HeapHead; } } }
enum MEMArena : unsigned int;
struct VPADTPData;
struct PADStatus {
    u16 buttons;
    s8 stickX;
    s8 stickY;
    s8 substickX;
    s8 substickY;
    u8 triggerLeft;
    u8 triggerRight;
    u8 analogA;
    u8 analogB;
    s8 error;
    u8 _b;
};
static_assert(sizeof(PADStatus) == 12, "Legacy pad status layout");

struct VPADStatus {
    u32 hold;
    u32 trigger;
    u32 release;
    float leftX;
    float leftY;
    float rightX;
    float rightY;
    u8 _1c[0x58];
    u8 error;
    u8 _75[0xb];
};
static_assert(sizeof(VPADStatus) == 0x80, "VPAD status layout");

void GX2SampleTopGPUCycle(unsigned long* pCycle);
void GX2SampleBottomGPUCycle(unsigned long* pCycle);
unsigned long OSGetSystemTime();
unsigned long OSTicksToMicroseconds(unsigned long ticks);
unsigned long OSTicksToMilliseconds(unsigned long ticks);
unsigned long OSMillisecondsToTicks(unsigned long long milliseconds);
unsigned long OSTicksToSeconds(unsigned long ticks);
unsigned long OSGetTick();
unsigned long OSGetTime();
unsigned long OSDiffTick(unsigned long start, unsigned long end);
unsigned int MEMGetHeapType(nn::lmem::detail::HeapHead* pHeap);
unsigned int MEMGetHeapEndAddress(nn::lmem::detail::HeapHead* pHeap);
unsigned int MEMGetHeapStartAddress(nn::lmem::detail::HeapHead* pHeap);
unsigned int MEMGetTotalFreeSizeForExpHeap(nn::lmem::detail::HeapHead* pHeap);
unsigned int MEMGetTrackingLeftInBlockHeap(nn::lmem::detail::HeapHead* pHeap);
unsigned int MEMGetAllocatableSizeForExpHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment);
unsigned int MEMGetAllocatableSizeForFrmHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment);
unsigned int MEMGetAllocatableSizeForBlockHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int alignment);
void* MEMAllocFromExpHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment);
void* MEMAllocFromFrmHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment);
void* MEMAllocFromBlockHeapEx(nn::lmem::detail::HeapHead* pHeap, unsigned int size, unsigned int alignment);
void MEMFreeToExpHeap(nn::lmem::detail::HeapHead* pHeap, void* pMemory);
void MEMFreeToBlockHeap(nn::lmem::detail::HeapHead* pHeap, void* pMemory);
void MEMFreeToFrmHeap(nn::lmem::detail::HeapHead* pHeap, unsigned int mode);
unsigned int MEMAddBlockHeapTracking(nn::lmem::detail::HeapHead* pHeap, unsigned char* pTracking, unsigned int size);
nn::lmem::detail::HeapHead* MEMGetBaseHeapHandle(MEMArena arena);
void GX2Shutdown();
void ProcUIClearCallbacks();
int getPlatformRegion();
void OSSetScreenCapturePermission(bool enabled);
void OSSleepMicroseconds(unsigned int microseconds);
void* VESSEL_malloc(unsigned int size, unsigned int alignment);
void* VESSEL_MALLOCEX(unsigned int size, unsigned int alignment);
void VESSEL_FREE(void* pMemory);
void OSEnableForegroundExit();
void OSSavesDone_ReadyToRelease();
void ProcUIDrawDoneRelease();
unsigned int OSGetCoreId();
unsigned int OSGetStackPointer();
void* OSBlockMove(unsigned int* pDestination, unsigned int* pSource, unsigned int size, bool flush);
void DCFlushRange(void* pMemory, unsigned int size);
unsigned int inverseBits(unsigned int value);
void MEMFreeToDefaultHeap(void* pMemory);
int VPADRead(int channel, VPADStatus* pStatus, unsigned int count, int* pError);
void VPADGetTPCalibratedPoint(int channel, VPADTPData* pOutput, const VPADTPData* pInput);
void PADRead(PADStatus* pStatus);
