/**
 * @file vfx_System.h
 * @brief VFX system and the runtime helpers it drives.
 */

#pragma once

#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/vfx/Heap.h>
#include <nn/vfx/System.h>
#include <nn/vfx/vfx_SuperStripe.h>

namespace nn {
namespace vfx {

void DumpEmitterInformation(Emitter* pEmitter, int isOutputGroupId);

namespace detail {

void SetStaticHeap(Heap* pHeap);
void SetDynamicHeap(Heap* pHeap);
Heap* GetDynamicHeap();
size_t GetAllocatedSizeFromStaticHeap();

void InitializeDelayFreeList(int count);
void FinalizeDelayFreeList();
void FlushDelayFreeList();

void SetSuppressOutputLog(bool isSuppress);
void OutputLog(const char* pFormat, ...);
void OutputWarning(const char* pFormat, ...);
void OutputError(const char* pFormat, ...);

void InitializeCurlNoise(gfx::Device* pDevice, Heap* pHeap);
void FinalizeCurlNoise(gfx::Device* pDevice);
size_t GetCurlNoiseTextureAllocatedSize();
bool RegisterCurlNoiseTextureViewToDescriptorPool(RegisterTextureViewSlot pFunc, void* pUserData);
void UnRegisterCurlNoiseTextureViewToDescriptorPool(UnregisterTextureViewSlot pFunc,
                                                    void* pUserData);

/** A gfx sampler shared between every emitter using the same settings. */
class TextureSampler {
public:
    static void InitializeSamplerTable(gfx::Device* pDevice, Heap* pHeap);
    static void FinalizeSamplerTable(gfx::Device* pDevice, Heap* pHeap);
    static TextureSampler* GetSamplerFromTable(ResTextureSampler* pResSampler);
    static bool RegisterSamplerToDescriptorPool(void* pFunc, void* pUserData);
    static void UnregisterSamplerFromDescriptorPool(void* pFunc, void* pUserData);

    const gfx::DescriptorSlot& GetDescriptorSlot() const { return m_DescriptorSlot; }

    u8 _0[0x28];
    gfx::DescriptorSlot m_DescriptorSlot;
};

class StripeSystem {
public:
    static int GetExtendedEndTimeForOneTimeEmitter(Emitter* pEmitter);
    static void InitializeSystem(Heap* pHeap, System* pSystem, BufferingMode bufferingMode,
                                 int stripeNum);
    static void FinalizeSystem(Heap* pHeap);
};

class ConnectionStripeSystem {
public:
    static void InitializeSystem(Heap* pHeap, System* pSystem, BufferingMode bufferingMode);
    static void FinalizeSystem(Heap* pHeap);
};

class AreaLoopSystem {
public:
    static void Initialize(System* pSystem);
};

}  // namespace detail
}  // namespace vfx
}  // namespace nn
