#pragma once

#include <nn/g3d/g3d_ResFile.h>
#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_Device.h>
#include <nn/gfx/gfx_ResShader.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/vfx/EmitterSet.h>

namespace nn {
namespace vfx {

struct Heap;
struct EmitterResource;
struct EmitterSetResource;
struct System;

namespace detail {
struct BinaryData;
struct BufferSizeCalculator;
struct ResNameTableRecord;
}  // namespace detail

class Resource {
public:
    Resource(gfx::Device*, Heap*, void*, gfx::MemoryPool*, size_t, size_t, int, System*, bool,
             Resource*);
    virtual ~Resource();

    void Trace(detail::BinaryData*);
    void Finalize(Heap*);
    void TraceGfxResTextureFile(detail::BinaryData*);
    void TracePrimitiveArray(detail::BinaryData*);
    void TraceG3dPrimitiveArray(detail::BinaryData*);
    void TraceShaderBinaryArray(detail::BinaryData*);
    void TraceEmitterSetArray(detail::BinaryData*, detail::BufferSizeCalculator*);
    void InitializeEmitterGraphicsResource(EmitterResource*);
    void FinalizeEmitterResource(EmitterResource*);
    void Unrelocate(void*);
    void RegisterTextureViewToDescriptorPool(bool (*)(gfx::DescriptorSlot*, const gfx::TextureView&,
                                                      void*),
                                             void*);
    void GetTextureDescriptorSlot(gfx::DescriptorSlot*, size_t) const;
    void SearchRecordFromResNameTable(detail::ResNameTableRecord*, size_t);
    void BindExternalResTextureFile(gfx::ResTextureFile*);
    void UnregisterTextureViewFromDescriptorPool(void (*)(gfx::DescriptorSlot*,
                                                          const gfx::TextureView&, void*),
                                                 void*);
    void BindExternalG3dResFile(g3d::ResFile*);
    void BindExternalResShaderFile(gfx::ResShaderFile*, gfx::ResShaderFile*);
    void InitializeEmitterSetResource(EmitterSetResource*, detail::BufferSizeCalculator*);
    void InitializeEmitterResource(EmitterResource*, detail::BinaryData*, int,
                                   detail::BufferSizeCalculator*);
    void GetPrimitive(size_t) const;
    void GetG3dPrimitive(size_t) const;
    s32 SearchEmitterSetId(const char*) const;
    void SearchEmitterSetIdWithFilePath(const char*) const;
    void BindResource(int, EmitterSetResource*);
    void UnbindResource(int);
    bool IsExistChildEmitter(int) const;
    bool IsNeedFade(int) const;
    void OutputResourceInfo() const;

    bool IsInitialized() const { return m_IsInitialized; }
    s32 GetEmitterSetNum() const { return m_EmitterSetNum; }

    EmitterSetResource* GetEmitterSetResource(s32 emitterSetId) const {
        return m_BindEmitterSetResArray[emitterSetId] != nullptr ?
                   m_BindEmitterSetResArray[emitterSetId] :
                   &m_EmitterSetResArray[emitterSetId];
    }

    const char* GetEmitterSetName(s32 emitterSetId) const {
        EmitterSetResource* resource = GetEmitterSetResource(emitterSetId);
        return resource != nullptr ? resource->GetName() : nullptr;
    }

    bool IsLoopEmitterSet(s32 emitterSetId) const {
        if (emitterSetId >= m_EmitterSetNum) {
            return false;
        }

        EmitterSetResource* resource = GetEmitterSetResource(emitterSetId);
        return resource != nullptr ? resource->m_IsLoop : false;
    }

    bool IsInfinityEmitterSet(s32 emitterSetId) const {
        if (emitterSetId >= m_EmitterSetNum) {
            return false;
        }

        EmitterSetResource* resource = GetEmitterSetResource(emitterSetId);
        return resource != nullptr ? resource->m_IsInfinity : false;
    }

    u8 _8;
    bool m_IsInitialized;
    u8 _a[0x4c - 0xa];
    s32 m_EmitterSetNum;
    u8 _50[0x58 - 0x50];
    EmitterSetResource* m_EmitterSetResArray;
    EmitterSetResource** m_BindEmitterSetResArray;
    u8 _68[0x540 - 0x68];
};

static_assert(sizeof(Resource) == 0x540);

}  // namespace vfx
}  // namespace nn
