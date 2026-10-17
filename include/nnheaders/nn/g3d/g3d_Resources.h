#pragma once
#include <nn/g3d/g3d_Bounding.h>

// Shared G3D resource and object declarations used by NintendoWare and AGL.

#include <attributes.h>
#include <cstring>
#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_BufferInfo.h>
#include <nn/gfx/gfx_SamplerInfo.h>
#include <nn/util/util_BitFlagSet.h>
#include <nn/gfx/gfx_DescriptorSlot.h>
#include <nn/gfx/gfx_MemoryPoolInfo.h>
#include <nn/gfx/gfx_ResTexture.h>
#include <nn/gfx/gfx_Texture.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/g3d/g3d_AnimObj.h>
#include <nn/g3d/g3d_Bounding.h>
#include <nn/g3d/g3d_Flag.h>
#include <nn/util/util_BinTypes.h>
#include <nn/util/util_BinaryFormat.h>
#include <nn/util/util_ResDic.h>

namespace nn::g3d {

class MaterialObj;
class ResSkeleton;
class ResSkeletalAnim;
class ResMaterialAnim;
class ResPerMaterialAnim;
class ResShapeAnim;
class ResSceneAnim;
struct ResBoneVisibilityAnim;
class ShapeObj;
class SkeletonObj;
class ViewVolume;
struct CullingContext;

typedef nn::gfx::detail::BufferImpl<nn::gfx::ApiVariationNvn8> BufferImpl;

class TextureRef {
public:
    TextureRef() : m_pTextureView(nullptr), m_DescriptorSlot(InvalidDescriptorSlot) {}
    TextureRef(const nn::gfx::TextureView* pTextureView, u64 descriptorSlot)
        : m_pTextureView(pTextureView), m_DescriptorSlot(descriptorSlot)
    {
    }

    const nn::gfx::TextureView* GetTextureView() const { return m_pTextureView; }
    u64 GetDescriptorSlot() const { return m_DescriptorSlot; }
    bool IsValid() const
    {
        return m_pTextureView != nullptr && m_DescriptorSlot != InvalidDescriptorSlot;
    }

    static const u64 InvalidDescriptorSlot = 0xFFFFFFFFFFFFFFFF;

private:
    const nn::gfx::TextureView* m_pTextureView;
    u64 m_DescriptorSlot;
};

class BindResult {
public:
    enum Flag { Flag_Success = 1 << 0, Flag_Failure = 1 << 16 };

    BindResult() : m_Flag(0), m_Pad(0) {}
    // flags records the success and failure bits for a single binding attempt.
    explicit BindResult(u32 flags) : m_Flag(flags), m_Pad(0) {}
    // result contributes success/failure bits from another resource binding operation.
    void Merge(const BindResult& result) { m_Flag |= result.m_Flag; }
    /** @brief Test whether any target bound successfully.
     * @return True when at least one binding succeeded. */
    bool IsAnySuccess() const { return (m_Flag & Flag_Success) != 0; }
    /** @brief Test whether binding failed without any successful targets.
     * @return True when only the failure flag is set. */
    bool IsFailure() const { return (m_Flag & (Flag_Success | Flag_Failure)) == Flag_Failure; }
    bool IsComplete() const { return (m_Flag & (Flag_Success | Flag_Failure)) == Flag_Success; }

private:
    u32 m_Flag;
    u32 m_Pad;
};

typedef TextureRef (*TextureBindCallback)(const char* name, void* pUserData);

inline void* GetTextureViewUserPtr(const nn::gfx::TextureView* pTextureView)
{
    return pTextureView->ToData()->userPtr.ptr;
}

struct ResSamplerData {
    nn::util::BinPtr pName;
    nn::util::BinPtr pGfxSampler;
    u8 _10[0x78 - 0x10];
};

struct ResShaderAssignData {
    nn::util::BinPtr pShaderArchiveName;
    nn::util::BinPtr pShadingModelName;
    nn::util::BinTPtr<nn::util::BinPtrToString> pAttribAssignArray;
    nn::util::BinTPtr<nn::util::ResDic> pAttribAssignDic;
    nn::util::BinTPtr<nn::util::BinPtrToString> pSamplerAssignArray;
    nn::util::BinTPtr<nn::util::ResDic> pSamplerAssignDic;
    nn::util::BinTPtr<nn::util::BinPtrToString> pOptionArray;
    nn::util::BinTPtr<nn::util::ResDic> pOptionDic;
    u8 _40[6];
    u16 optionCount;
};
class ResShaderAssign : public nn::util::AccessorBase<ResShaderAssignData> {
public:
    // index selects a shader option name in the assignment dictionary.
    const char* GetOptionName(int index) const {
        const nn::util::ResDic* dictionary = pOptionDic.Get();
        return (dictionary != nullptr) ? dictionary->GetKey(index).data() : nullptr;
    }
    const char* GetShaderArchiveName() const {
        return static_cast<const char*>(pShaderArchiveName.Get()) + 2;
    }
    const char* GetShadingModelName() const {
        return static_cast<const char*>(pShadingModelName.Get()) + 2;
    }
    const char* FindShaderOption(const char* pName) const;
    int GetShaderOptionCount() const { return optionCount; }
    // index selects the value of an assigned shader option.
    const char* GetShaderOption(int index) const {
        return pOptionArray.Get()[index].Get()->GetData();
    }
};
class ResShaderParam;
// destination receives converted source data; parameter describes its layout; dependency is optional context.
using ShaderParamConvertCallback = size_t (*)(void* destination, const void* source,
                                             const ResShaderParam* parameter, const void* dependency);
struct ResShaderParamData {
    ShaderParamConvertCallback callback;
    nn::util::BinPtrToString name;
    u8 type;
    u8 sourceSize;
    u16 sourceOffset;
    s32 offset;
    u16 index;
    u16 dependencyIndex;
    u8 _1c[4];
};

class ResShaderParam : public nn::util::AccessorBase<ResShaderParamData> {
public:
    enum Type : int { Type_Bool, Type_Srt2d = 28, Type_Srt3d, Type_TexSrt, Type_TexSrtEx };
    // type selects the source parameter representation whose byte size is returned.
    static size_t GetSrcSize(Type type);
    // type selects the GPU parameter representation whose byte size is returned.
    static size_t GetSize(Type type);
    // source holds parameter storage; dependency supplies the referenced parameter storage.
    bool SetDependPointer(void* source, const void* dependency) const;
    // dependency receives the stored pointer, or null when source has no pointer slot.
    bool GetDependPointer(void** dependency, const void* source) const;
    // destination receives converted source values; parameter describes them and user is callback context.
    static size_t ConvertSrt2dCallback(void* destination, const void* source, const ResShaderParam* parameter, const void* user);
    static size_t ConvertSrt3dCallback(void* destination, const void* source, const ResShaderParam* parameter, const void* user);
    static size_t ConvertSrt2dExCallback(void* destination, const void* source, const ResShaderParam* parameter, const void* user);
    static size_t ConvertTexSrtCallback(void* destination, const void* source, const ResShaderParam* parameter, const void* user);
    static size_t ConvertTexSrtExCallback(void* destination, const void* source, const ResShaderParam* parameter, const void* user);
    // destination receives the GPU representation of source; swap enables byte swapping.
    template <bool swap> void Convert(void* destination, const void* source) const;

    u16 GetSrcOffset() const { return sourceOffset; }
    s32 GetOffset() const { return offset; }
    // pCallback converts the source value of this parameter into its GPU representation.
    void SetConvertCallback(ShaderParamConvertCallback pCallback) { callback = pCallback; }
    int GetIndex() const { return index; }
};

class ResRenderInfo {
public:
    enum Type { Type_Int, Type_Float, Type_String };

    const char* GetName() const { return m_pName + 2; }
    int GetArrayLength() const { return m_ArrayLength; }
    Type GetType() const { return static_cast<Type>(m_Type); }
    const s32* GetInt() const { return m_pIntArray; }
    const float* GetFloat() const { return m_pFloatArray; }
    const char* GetString(int index) const { return m_pStringArray[index] + 2; }

private:
    const char* m_pName;
    union {
        const s32* m_pIntArray;
        const float* m_pFloatArray;
        const char* const* m_pStringArray;
    };
    u16 m_ArrayLength;
    u8 m_Type;
    u8 m_Reserved[5];
};

struct ResMaterialData {
    u32 signature;
    u32 flags;
    nn::util::BinPtr pName;
    nn::util::BinTPtr<ResRenderInfo> pRenderInfoArray;
    nn::util::BinTPtr<nn::util::ResDic> pRenderInfoDic;
    nn::util::BinTPtr<ResShaderAssignData> pShaderAssign;
    nn::util::BinTPtr<const nn::gfx::TextureView*> pTextureArray;
    nn::util::BinTPtr<nn::util::BinPtrToString> pTextureNameArray;
    nn::util::BinTPtr<ResSamplerData> pSamplerArray;
    nn::util::BinTPtr<nn::gfx::SamplerInfo> pSamplerInfoArray;
    nn::util::BinTPtr<nn::util::ResDic> pSamplerDic;
    nn::util::BinTPtr<ResShaderParamData> pShaderParamArray;
    nn::util::BinTPtr<nn::util::ResDic> pShaderParamDic;
    nn::util::BinPtr pSourceParamData;
    u8 _68[0x10];
    nn::util::BinTPtr<u32> pVolatileParamFlags;
    nn::util::BinPtr pUserPtr;
    nn::util::BinTPtr<u64> pSamplerSlotArray;
    nn::util::BinTPtr<u64> pTextureSlotArray;
    u16 index;
    u16 _9a;
    u8 samplerCount;
    u8 textureCount;
    u16 shaderParamCount;
    u16 volatileParamCount;
    u16 sourceParamSize;
    u16 materialBlockSize;
    u8 _a6[2];
};

class ResMaterial : public nn::util::AccessorBase<ResMaterialData> {
public:
    BindResult BindTexture(TextureBindCallback callback, void* user);
    bool ForceBindTexture(const TextureRef& texture, const char* name);
    void ReleaseTexture();
    void Setup(nn::gfx::Device* device);
    void Cleanup(nn::gfx::Device* device);
    void Reset();
    void Reset(u32 guard);
    // name selects a shader parameter or sampler in its resource dictionary.
    int FindShaderParamIndex(const char* name) const {
        const nn::util::ResDic* dictionary = pShaderParamDic.Get();
        return (dictionary != nullptr) ? dictionary->FindIndex(name) : nn::util::ResDic::Npos;
    }
    int FindSamplerIndex(const char* name) const {
        const nn::util::ResDic* dictionary = pSamplerDic.Get();
        return (dictionary != nullptr) ? dictionary->FindIndex(name) : nn::util::ResDic::Npos;
    }
    int GetIndex() const { return index; }
    const char* GetName() const { return static_cast<const char*>(pName.Get()) + 2; }
    int GetSamplerCount() const { return ToData().samplerCount; }
    int GetTextureCount() const { return ToData().textureCount; }

    const nn::gfx::TextureView* GetTextureView(int index) const
    {
        return ToData().pTextureArray.Get()[index];
    }
    const char* GetTextureName(int index) const
    {
        return ToData().pTextureNameArray.Get()[index].Get()->GetData();
    }
    u64 GetTextureDescriptorSlot(int index) const
    {
        return ToData().pTextureSlotArray.Get()[index];
    }

    void ForceBindTexture(int index, const TextureRef& rRef)
    {
        ToData().pTextureArray.Get()[index] = rRef.GetTextureView();
        ToData().pTextureSlotArray.Get()[index] = rRef.GetDescriptorSlot();
    }
    void ReleaseTexture(int index)
    {
        ToData().pTextureArray.Get()[index] = nullptr;
        ToData().pTextureSlotArray.Get()[index] = TextureRef::InvalidDescriptorSlot;
    }

    const ResShaderAssign* GetShaderAssign() const {
        return reinterpret_cast<const ResShaderAssign*>(ToData().pShaderAssign.Get());
    }
    const ResShaderParam* GetShaderParam(int index) const {
        return reinterpret_cast<const ResShaderParam*>(&ToData().pShaderParamArray.Get()[index]);
    }
    ResShaderParam* GetShaderParam(int index) {
        return reinterpret_cast<ResShaderParam*>(&ToData().pShaderParamArray.Get()[index]);
    }
    // name selects a shader parameter; return null when its dictionary has no entry.
    ResShaderParam* FindShaderParam(const char* name) {
        int index = FindShaderParamIndex(name);
        return index == nn::util::ResDic::Npos ? nullptr : GetShaderParam(index);
    }
    const ResRenderInfo* FindRenderInfo(const char* pName) const;
};

struct ResVertexAttribData {
    u8 _0[0x8];
    u32 format;
    u16 offset;
    u8 bufferIndex;
    u8 flags;
};

struct ResVertexBufferInfoData {
    u32 size;
    u8 _4[0xc];
};

struct ResVertexBufferStrideData {
    u32 stride;
    u8 _4[0xc];
};

struct ResVertexData {
    u8 _0[0x8];
    nn::util::BinTPtr<ResVertexAttribData> pAttribArray;
    nn::util::BinTPtr<nn::util::ResDic> pAttribDic;
    nn::util::BinTPtr<nn::gfx::MemoryPool> pMemoryPool;
    nn::util::BinTPtr<nn::gfx::Buffer> pBufferObjects;
    nn::util::BinTPtr<nn::gfx::Buffer*> pVertexBufferArray;
    nn::util::BinTPtr<ResVertexBufferInfoData> pVertexBufferInfoArray;
    nn::util::BinTPtr<ResVertexBufferStrideData> pVertexBufferStrideArray;
    nn::util::BinPtr pUserPtr;
    u32 memoryPoolOffset;
    u8 attribCount;
    u8 bufferCount;
    u8 _4e[0xa];
};

class ResVertex : public nn::util::AccessorBase<ResVertexData> {
public:
    /**
     * @brief Access the graphics buffer description stored for a vertex stream.
     * @param index Buffer index in the range [0, bufferCount).
     * @return Pointer to the selected buffer description in the resource.
     */
    const gfx::BufferInfo* GetBufferInfo(ptrdiff_t index) const {
        return reinterpret_cast<const gfx::BufferInfo*>(&pVertexBufferInfoArray.Get()[index]);
    }
    /**
     * @brief Access a writable graphics buffer description for a vertex stream.
     * @param index Buffer index in the range [0, bufferCount).
     * @return Pointer to the selected buffer description in the resource.
     */
    gfx::BufferInfo* GetBufferInfo(ptrdiff_t index) {
        return reinterpret_cast<gfx::BufferInfo*>(&pVertexBufferInfoArray.Get()[index]);
    }

    void Setup(nn::gfx::Device* device);
    void Setup(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset);
    void Cleanup(nn::gfx::Device* device);
    void Reset();
    void Reset(u32 guard);
    void CalculateDynamicVertexBufferIndex(nn::util::BitFlagSet<255, void>* indices) const;
};

struct ResSubMesh { u32 offset; u32 count; };
class ResMesh {
public:
    void Setup(nn::gfx::Device* device);
    void Setup(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset);
    void Cleanup(nn::gfx::Device* device);
    void DrawSubMesh(nn::gfx::CommandBuffer* command, int first, int count, int instances) const;
    void DrawSubMesh(nn::gfx::CommandBuffer* command, int first, int count, int instances, int baseInstance) const;
    s32 GetSubMeshCount() const { return subMeshCount; }

    ResSubMesh* subMeshes;
    nn::gfx::MemoryPool* memoryPool;
    nn::gfx::Buffer* buffer;
    nn::gfx::BufferInfo* bufferInfo;
    u32 memoryPoolOffset;
    nn::gfx::PrimitiveTopology topology;
    nn::gfx::IndexFormat indexFormat;
    u32 indexCount;
    u32 baseVertex;
    u16 subMeshCount;
    u16 _36;
};

struct ResKeyShape { u8 attribIndices[18]; u8 _12[2]; };
struct ResShapeData {
    u32 signature;
    u32 flags;
    nn::util::BinPtr pName;
    nn::util::BinTPtr<ResVertex> pVertex;
    nn::util::BinTPtr<ResMesh> pMeshArray;
    nn::util::BinPtr pSkinBoneIndexArray;
    nn::util::BinTPtr<ResKeyShape> pKeyShapeArray;
    nn::util::BinTPtr<nn::util::ResDic> pKeyShapeDic;
    nn::util::BinTPtr<Bounding> pBoundingArray;
    u8 _40[0x48 - 0x40];
    nn::util::BinPtr pUserPtr;
    u16 index;
    u16 materialIndex;
    u16 boneIndex;
    u8 _56[4];
    u8 vertexSkinCount;
    u8 meshCount;
    u8 keyShapeCount;
    u8 _5d[3];
};

class ResShape : public nn::util::AccessorBase<ResShapeData> {
public:
    // name identifies a key shape in the resource dictionary.
    int FindKeyShapeIndex(const char* name) const {
        const nn::util::ResDic* dictionary = pKeyShapeDic.Get();
        if (dictionary == nullptr) return nn::util::ResDic::Npos;
        return dictionary->FindIndex(name);
    }
    int GetIndex() const { return index; }
    const char* GetName() const { return static_cast<const char*>(pName.Get()) + 2; }
    s32 GetMaterialIndex() const { return materialIndex; }
    const ResMesh* GetMesh() const { return pMeshArray.Get(); }
    const ResMesh* GetMesh(int meshIndex) const { return &pMeshArray.Get()[meshIndex]; }
    int GetMeshCount() const { return meshCount; }
    /**
     * @brief Get the bone that supplies the shape's rigid transform.
     * @return Bone index in the model skeleton.
     */
    int GetBoneIndex() const { return boneIndex; }
    /**
     * @brief Get the number of skinning influences per vertex.
     * @return Skinning influence count; zero identifies a rigid shape.
     */
    int GetVertexSkinCount() const { return vertexSkinCount; }
    const Bounding* GetBoundingArray() const { return pBoundingArray.Get(); }

    void ActivateDynamicVertexAttrForShapeAnim();
    void Setup(nn::gfx::Device* device);
    void Setup(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset);
    void Cleanup(nn::gfx::Device* device);
    void Reset();
    void Reset(u32 guard);
};

struct ResModelData {
    u8 _0[0x18];
    nn::util::BinTPtr<ResSkeleton> pSkeleton;
    nn::util::BinTPtr<ResVertex> pVertexArray;
    nn::util::BinTPtr<ResShape> pShapeArray;
    nn::util::BinTPtr<nn::util::ResDic> pShapeDic;
    nn::util::BinTPtr<ResMaterial> pMaterialArray;
    u8 _40[8];
    nn::util::BinTPtr<nn::util::ResDic> pMaterialDic;
    u8 _50[0x10];
    nn::util::BinPtr pUserPtr;
    u16 vertexCount;
    u16 shapeCount;
    u16 materialCount;
    u8 _6e[0x78 - 0x6e];
};

class ResModel : public nn::util::AccessorBase<ResModelData> {
public:
    // name selects a material; missing dictionary entries return null.
    NOINLINE const ResMaterial* FindMaterial(const char* name) const {
        const nn::util::ResDic* dictionary = pMaterialDic.Get();
        int index = (dictionary != nullptr) ? dictionary->FindIndex(name) : nn::util::ResDic::Npos;
        if (index == nn::util::ResDic::Npos) return nullptr;
        return &pMaterialArray.Get()[index];
    }
    // name identifies a shape in this model; return null if its dictionary has no entry.
    NOINLINE const ResShape* FindShape(const char* name) const {
        const nn::util::ResDic* dictionary = pShapeDic.Get();
        int index = (dictionary != nullptr) ? dictionary->FindIndex(name) : nn::util::ResDic::Npos;
        if (index == nn::util::ResDic::Npos) return nullptr;
        return &pShapeArray.Get()[index];
    }

    BindResult BindTexture(TextureBindCallback callback, void* user);
    bool ForceBindTexture(const TextureRef& texture, const char* name);
    void ReleaseTexture();
    void ActivateDynamicVertexAttrForShapeAnim();
    void Setup(nn::gfx::Device* device);
    void Setup(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset);
    void Cleanup(nn::gfx::Device* device);
    void Reset();
    void Reset(u32 guard);
    int GetMaterialCount() const { return ToData().materialCount; }
    ResMaterial* GetMaterial(int index) { return &ToData().pMaterialArray.Get()[index]; }

    int FindShapeIndex(const char* pName) const {
        const nn::util::ResDic* pDic = ToData().pShapeDic.Get();
        return pDic ? pDic->FindIndex(pName) : nn::util::ResDic::Npos;
    }
    s32 GetShapeCount() const { return ToData().shapeCount; }
    const ResShape* GetShape(int index) const { return &ToData().pShapeArray.Get()[index]; }
    const ResSkeleton* GetSkeleton() const { return ToData().pSkeleton.Get(); }
    void SetUserPtr(void* ptr) { ToData().pUserPtr.Set(ptr); }
    void* GetUserPtr() { return ToData().pUserPtr.Get(); }
    const void* GetUserPtr() const { return ToData().pUserPtr.Get(); }
};

struct ResExternalFileData {
    nn::util::BinPtr pData;
    u32 size;
    u32 _c;
};

struct ResFileData {
    nn::util::BinaryFileHeader fileHeader;
    u8 _20[0x28 - 0x20];
    nn::util::BinTPtr<ResModel> pModelArray;
    u8 _30[0x58 - 0x30];
    nn::util::BinTPtr<ResSkeletalAnim> pSkeletalAnimArray;
    nn::util::BinPtr pSkeletalAnimDic;
    nn::util::BinTPtr<ResMaterialAnim> pMaterialAnimArray;
    nn::util::BinPtr pMaterialAnimDic;
    nn::util::BinTPtr<ResBoneVisibilityAnim> pBoneVisibilityAnimArray;
    nn::util::BinPtr pBoneVisibilityAnimDic;
    nn::util::BinTPtr<ResShapeAnim> pShapeAnimArray;
    nn::util::BinPtr pShapeAnimDic;
    nn::util::BinTPtr<ResSceneAnim> pSceneAnimArray;
    nn::util::BinPtr pSceneAnimDic;
    nn::util::BinTPtr<nn::gfx::MemoryPool> pMemoryPool;
    nn::util::BinTPtr<nn::gfx::MemoryPoolInfo> pMemoryPoolInfo;
    nn::util::BinTPtr<ResExternalFileData> pExternalFileArray;
    nn::util::BinTPtr<nn::util::ResDic> pExternalFileDic;
    nn::util::BinPtr pUserPtr;
    u8 _d0[0xdc - 0xd0];
    u16 modelCount;
    u8 _de[4];
    u16 skeletalAnimCount;
    u16 materialAnimCount;
    u16 boneVisibilityAnimCount;
    u16 shapeAnimCount;
    u16 sceneAnimCount;
    u16 externalFileCount;
};

class ResFile : public nn::util::AccessorBase<ResFileData> {
public:
    static bool IsValid(const void* file);
    void Relocate();
    void Unrelocate();
    static ResFile* ResCast(void* pFile);
    void ReleaseTexture();
    void Setup(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset, size_t size);
    void Reset();

    void Setup(nn::gfx::Device* pDevice);
    void Cleanup(nn::gfx::Device* pDevice);
    BindResult BindTexture(TextureBindCallback pCallback, void* pUserData);

    int GetModelCount() const { return ToData().modelCount; }
    int GetSkeletalAnimCount() const { return ToData().skeletalAnimCount; }
    int GetMaterialAnimCount() const { return ToData().materialAnimCount; }
    int GetBoneVisibilityAnimCount() const { return ToData().boneVisibilityAnimCount; }
    ResModel* GetModel(int index) { return &ToData().pModelArray.Get()[index]; }

    // Defined in g3d_ResSceneAnim.h.
    const ResSceneAnim* FindSceneAnim(const char* pName) const;

    int FindExternalFileIndex(const char* pName) const
    {
        const nn::util::ResDic* pDic = ToData().pExternalFileDic.Get();
        return (pDic != nullptr) ? pDic->FindIndex(pName) : nn::util::ResDic::Npos;
    }
    int GetExternalFileCount() const { return ToData().externalFileCount; }
    const char* GetExternalFileName(int index) const
    {
        const nn::util::ResDic* pDic = ToData().pExternalFileDic.Get();
        return (pDic != nullptr) ? pDic->GetKey(index).data() : nullptr;
    }
    const ResExternalFileData* GetExternalFile(int index) const
    {
        return &ToData().pExternalFileArray.Get()[index];
    }
    const ResExternalFileData* FindExternalFile(const char* pName) const
    {
        int index = FindExternalFileIndex(pName);
        return index != nn::util::ResDic::Npos ? GetExternalFile(index) : nullptr;
    }
};

struct ResMaterialAnimData {
    u32 signature;
    u16 flags;
    u8 _6[2];
    nn::util::BinPtrToString name;
    u8 _10[8];
    const ResModel* boundModel;
    u16* bindIndices;
    ResPerMaterialAnim* materialAnims;
    nn::util::BinTPtr<const nn::gfx::TextureView*> pTextureArray;
    nn::util::BinTPtr<nn::util::BinPtrToString> pTextureNameArray;
    u8 _40[0x50 - 0x40];
    nn::util::BinTPtr<u64> pTextureSlotArray;
    s32 frameCount;
    u32 bakedSize;
    u16 _60;
    u16 materialAnimCount;
    u16 curveCount;
    u16 shaderParamAnimCount;
    u16 texturePatternAnimCount;
    u16 _6a;
    u16 textureCount;
};

class ResMaterialAnim : public nn::util::AccessorBase<ResMaterialAnimData> {
public:
    BindResult PreBind(const ResModel* model);
    BindResult BindCheck(const ResModel* model) const;
    bool ForceBindTexture(const TextureRef& texture, const char* name);
    bool BakeCurve(void* buffer, size_t size);
    void* ResetCurve();
    BindResult BindTexture(TextureBindCallback callback, void* user);
    void ReleaseTexture();
    void Reset();
    // index selects a texture descriptor in the animation resource.
    u64 GetTextureDescriptorSlot(int index) const { return pTextureSlotArray.Get()[index]; }
    int GetTextureCount() const { return ToData().textureCount; }
    int GetPerMaterialAnimCount() const { return ToData().materialAnimCount; }
    int GetCurveCount() const { return ToData().curveCount; }
    int GetParamAnimCount() const {
        return ToData().shaderParamAnimCount + ToData().texturePatternAnimCount;
    }
    bool IsCurveBaked() const { return ToData().flags & 1; }
    bool IsLooped() const { return ToData().flags & 4; }
    int GetFrameCount() const { return ToData().frameCount; }
    const char* GetName() const { return ToData().name.Get()->GetData(); }
    const nn::gfx::TextureView* GetTextureView(int index) const
    {
        return ToData().pTextureArray.Get()[index];
    }
    const char* GetTextureName(int index) const
    {
        return ToData().pTextureNameArray.Get()[index].Get()->GetData();
    }
    void ForceBindTexture(int index, const TextureRef& rRef)
    {
        ToData().pTextureArray.Get()[index] = rRef.GetTextureView();
        ToData().pTextureSlotArray.Get()[index] = rRef.GetDescriptorSlot();
    }
    void ReleaseTexture(int index)
    {
        ToData().pTextureArray.Get()[index] = nullptr;
        ToData().pTextureSlotArray.Get()[index] = TextureRef::InvalidDescriptorSlot;
    }
};

namespace detail {
struct WorkMemoryBlock {
    size_t size;
    size_t alignment;
    void* pointer;
    ptrdiff_t offset;
    // bytes is the requested size; each block starts at an eight-byte boundary.
    void Initialize(size_t bytes) { size = bytes; alignment = 8; pointer = nullptr; offset = -1; }
    /**
     * @brief Reset a workspace block with a specified size and alignment.
     * @param bytes Required storage size in bytes; zero denotes an unused block.
     * @param requiredAlignment Nonzero power-of-two byte alignment for this block.
     */
    void Initialize(size_t bytes, size_t requiredAlignment) {
        size = bytes;
        alignment = requiredAlignment;
        pointer = nullptr;
        offset = -1;
    }
    // totalSize and requiredAlignment accumulate workspace requirements; this block receives its offset.
    void AppendTo(size_t& totalSize, size_t& requiredAlignment) {
        if (size != 0) {
            size_t aligned = (totalSize + 7) & ~size_t(7);
            requiredAlignment = 8;
            totalSize = aligned + size;
            offset = aligned;
        }
    }
    // buffer is the base of the allocated work area; empty blocks return null.
    void* GetPointer(void* buffer) const { return (buffer != nullptr) && size ? static_cast<u8*>(buffer) + offset : nullptr; }
    // T selects the element type of the block within buffer.
    template <class T> T* GetPointer(void* buffer) const { return static_cast<T*>(GetPointer(buffer)); }
};
/**
 * @brief Assign a workspace block offset and update the allocation requirements.
 * @param block Block whose size is known and whose offset is assigned; empty blocks are skipped.
 * @param size Accumulated workspace size, updated to include the block.
 * @param alignment Accumulated alignment requirement, updated if the block requires more alignment.
 * @param blockAlignment Nonzero power-of-two alignment required by this block, in bytes.
 */
inline void AppendWorkspaceBlock(WorkMemoryBlock& block, size_t& size, size_t& alignment,
                                 size_t blockAlignment) {
    if (block.size != 0) {
        size_t start = (size + blockAlignment - 1) & -blockAlignment;
        alignment = alignment < blockAlignment ? blockAlignment : alignment;
        size = start + block.size;
        block.offset = start;
    }
}


}
class MaterialObj {
public:
    struct InitializeArgument {
        const ResMaterial* resource;
        int bufferCount;
        size_t memorySize;
        size_t memoryAlignment;
        detail::WorkMemoryBlock blocks[5];
        void CalculateMemorySize();
    };
    bool Initialize(const InitializeArgument& argument, void* memory, size_t size);
    void InitializeDependPointer();
    size_t GetBlockBufferAlignment(nn::gfx::Device* device) const;
    size_t CalculateBlockBufferSize(nn::gfx::Device* device) const;
    bool SetupBlockBuffer(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset, size_t size);
    void SetupBlockBufferImpl(nn::gfx::Device* device, nn::gfx::MemoryPool* pool, ptrdiff_t offset, size_t size);
    void CleanupBlockBuffer(nn::gfx::Device* device);
    void ResetDirtyFlags();
    void CalculateMaterial(int bufferIndex);
    template <bool swap> NOINLINE void ConvertDirtyParams(void* destination, u32* dirtyFlags);
    // material and index identify the texture slot that changed.
    using TextureChangeCallback = void (*)(MaterialObj* material, int index);
    /**
     * @brief Set the callback for texture changes on this material.
     * @param callback Callback receiving the material and texture slot; nullptr disables notifications.
     */
    void SetTextureChangeCallback(TextureChangeCallback callback) { m_pTextureChangeCallback = callback; }

    /**
     * @brief Construct an empty material object without allocated GPU or working storage.
     */
    MaterialObj() : m_pRes(nullptr), m_Flag(0), m_BufferingCount(0), m_DirtyFlags{},
        m_pMemoryPool(nullptr), m_MemoryPoolOffset(0), m_pMaterialBlockArray(nullptr),
        m_pParamSource(nullptr), m_ppTextureArray(nullptr), m_pTextureSlotArray(nullptr),
        m_MaterialBlockSize(0), m_pCallbackUserData(nullptr), m_pWorkMemory(nullptr),
        m_pTextureChangeCallback(nullptr) {}

    const ResMaterial* GetResource() const { return m_pRes; }
    // name selects a shader parameter in the material's resource dictionary.
    int FindShaderParamIndex(const char* name) const {
        return m_pRes->FindShaderParamIndex(name);
    }
    // paramIndex selects a shader parameter whose source value is returned.
    template <typename T>
    const T* GetShaderParam(int paramIndex) const
    {
        const ResShaderParam* pParam = m_pRes->GetShaderParam(paramIndex);
        return reinterpret_cast<const T*>(static_cast<const u8*>(m_pParamSource) +
                                          pParam->GetSrcOffset());
    }
    bool IsBlockBufferValid() const { return m_Flag & Flag_BlockBufferValid; }

    BufferImpl* GetMaterialBlock(int bufferIndex)
    {
        if (!IsBlockBufferValid())
        {
            return nullptr;
        }
        return (m_pMaterialBlockArray != nullptr) ? &m_pMaterialBlockArray[bufferIndex] : nullptr;
    }

    size_t GetMaterialBlockSize() const { return m_MaterialBlockSize; }
    int GetBufferingCount() const { return m_BufferingCount; }

    const nn::gfx::TextureView* GetTextureView(int index) const { return m_ppTextureArray[index]; }

    /**
     * @brief Replace a material texture and notify its change callback when needed.
     * @param index Sampler index within the material texture table.
     * @param rRef Texture view and descriptor slot to install.
     */
    void SetTexture(int index, const TextureRef& rRef)
    {
        const nn::gfx::TextureView* pOldView = m_ppTextureArray[index];
        const u64& rOldSlot = m_pTextureSlotArray[index];
        m_ppTextureArray[index] = rRef.GetTextureView();
        m_pTextureSlotArray[index] = rRef.GetDescriptorSlot();
        if ((m_pTextureChangeCallback != nullptr) &&
            (pOldView != rRef.GetTextureView() || std::memcmp(&rOldSlot, &m_pTextureSlotArray[index], sizeof(rOldSlot)) != 0))
        {
            m_pTextureChangeCallback(this, index);
        }
    }

    /**
     * @brief Mark a shader parameter and its dependent parameter dirty and expose its source data.
     * @param paramIndex Shader parameter index within the material resource.
     * @tparam T Type matching the selected shader parameter's source representation.
     * @return Writable pointer to the parameter source value.
     */
    template <typename T>
    T* EditShaderParam(int paramIndex) {
        const ResShaderParam* pParam = m_pRes->GetShaderParam(paramIndex);
        if (pParam->GetOffset() >= 0) {
            m_DirtyFlags.mPending[paramIndex >> 5] |= 1 << paramIndex;
            m_DirtyFlags.mFlags |= 1;
        }
        int dependedIndex = pParam->GetIndex();
        if (m_pRes->GetShaderParam(dependedIndex)->GetOffset() >= 0) {
            m_DirtyFlags.mPending[dependedIndex >> 5] |= 1 << dependedIndex;
            m_DirtyFlags.mFlags |= 1;
        }
        return reinterpret_cast<T*>(static_cast<u8*>(m_pParamSource) + pParam->GetSrcOffset());
    }

private:
    enum Flag { Flag_BlockBufferValid = 1 << 0 };

    const ResMaterial* m_pRes;
    u16 m_Flag;
    u8 m_BufferingCount;
    u8 _b[5];
    detail::FlagSet m_DirtyFlags;
    nn::gfx::MemoryPool* m_pMemoryPool;
    ptrdiff_t m_MemoryPoolOffset;
    BufferImpl* m_pMaterialBlockArray;
    void* m_pParamSource;
    const nn::gfx::TextureView** m_ppTextureArray;
    u64* m_pTextureSlotArray;
    size_t m_MaterialBlockSize;
    void* m_pCallbackUserData;
    void* m_pWorkMemory;
    TextureChangeCallback m_pTextureChangeCallback;
};

class MaterialAnimObj : public ModelAnimObj {
public:
    struct InitializeArgument {
        int materialCount;
        int materialAnimCount;
        int paramAnimCount;
        int textureCount;
        int curveCount;
        bool cacheEnabled;
        bool cacheAvailable;
        size_t memorySize;
        size_t memoryAlignment;
        detail::WorkMemoryBlock blocks[6];
        void CalculateMemorySize();
    };
    // Builder accumulates the capacities needed by a set of models and animations.
    class Builder : public InitializeArgument {
    public:
        /**
         * @brief Initialize unset capacities and enable curve caching by default.
         */
        Builder() {
            curveCount = -1;
            materialCount = -1;
            materialAnimCount = -1;
            paramAnimCount = -1;
            textureCount = -1;
            cacheEnabled = true;
            cacheAvailable = false;
            memorySize = 0;
            memoryAlignment = 0;

            for (int i = 0; i < 6; ++i) {
                blocks[i].Initialize(0);
            }
        }
        /**
         * @brief Reserve bindings for the target model.
         * @param model Model resource whose material count determines the target capacity.
         */
        void Reserve(const ResModel* model) { materialCount = model->GetMaterialCount(); }
        /**
         * @brief Increase capacities to accommodate an animation resource.
         * @param resource Non-null animation resource whose counts must fit the workspace.
         */
        void Reserve(const ResMaterialAnim* resource) {
            int count = resource->GetPerMaterialAnimCount();
            materialAnimCount = materialAnimCount < count ? count : materialAnimCount;
            count = resource->GetParamAnimCount();
            paramAnimCount = paramAnimCount < count ? count : paramAnimCount;
            count = resource->GetCurveCount();
            curveCount = curveCount < count ? count : curveCount;
            count = resource->GetTextureCount();
            textureCount = textureCount < count ? count : textureCount;
            cacheAvailable |= !resource->IsCurveBaked();
        }
        /**
         * @brief Get the calculated workspace requirement.
         * @return Workspace size in bytes after CalculateMemorySize.
         */
        size_t GetWorkMemorySize() const { return memorySize; }
        /**
         * @brief Initialize an animation object using the calculated layout.
         * @param object Non-null animation object to initialize.
         * @param memory Workspace aligned to the calculated requirement.
         * @param size Available bytes in memory; must cover the calculated requirement.
         * @return True if the capacities and workspace are valid.
         */
        bool Build(MaterialAnimObj* object, void* memory, size_t size) const {
            return object->Initialize(*this, memory, size);
        }
    };

    /**
     * @brief Construct an uninitialized material animation object.
     */
    MaterialAnimObj()
        : m_pRes(nullptr), m_pMaterialAnims(nullptr), m_MaterialAnimCapacity(0), m_ParamAnimCapacity(0),
          m_TextureCapacity(0), m_CurveCapacity(0), m_pSubBindIndices(nullptr),
          m_ppTextureArray(nullptr), m_pTextureSlotArray(nullptr) {}
    /**
     * @brief Destroy the object without releasing caller-owned workspace.
     */
    virtual ~MaterialAnimObj() {}
    bool Initialize(const InitializeArgument& argument, void* memory, size_t size);
    void SetResource(const ResMaterialAnim* resource);
    virtual BindResult Bind(const ResModel* model);
    virtual BindResult Bind(const ModelObj* model);
    virtual void BindFast(const ResModel* model);
    virtual void ClearResult();
    virtual void Calculate();
    virtual void ApplyTo(ModelObj* model) const;
    void RevertTo(ModelObj* pModel) const;

    /**
     * @brief Get the selected animation resource.
     * @return Current animation resource, or nullptr before SetResource.
     */
    const ResMaterialAnim* GetResource() const { return m_pRes; }
    /**
     * @brief Get a texture view from the animation texture table.
     * @param index Texture index below the selected resource's texture count.
     * @return Bound texture view, which may be nullptr.
     */
    const nn::gfx::TextureView* GetTextureView(int index) const { return m_ppTextureArray[index]; }
    /**
     * @brief Get a texture view and its descriptor binding.
     * @param index Texture index below the selected resource's texture count.
     * @return Texture reference containing the current view and descriptor.
     */
    TextureRef GetTexture(int index) const {
        return TextureRef(m_ppTextureArray[index], m_pTextureSlotArray[index]);
    }
    /**
     * @brief Replace an animation texture binding.
     * @param index Texture index below the selected resource's texture count.
     * @param rRef Texture view and descriptor slot to assign.
     */
    void SetTexture(int index, const TextureRef& rRef)
    {
        m_ppTextureArray[index] = rRef.GetTextureView();
        m_pTextureSlotArray[index] = rRef.GetDescriptorSlot();
    }

private:
    BindResult SubBind(const ResPerMaterialAnim* pAnim, const ResMaterial* pMaterial, int subBindIndex);
    BindResult SubBindFast(const ResPerMaterialAnim* pAnim, int subBindIndex);
    void ApplyTo(MaterialObj* pMaterial, const ResPerMaterialAnim* pAnim, int subBindIndex) const;
    void RevertTo(MaterialObj* pMaterial, const ResPerMaterialAnim* pAnim, int subBindIndex) const;
    template <bool cached> void CalculateMaterialImpl(const ResPerMaterialAnim* pAnim, float frame, int& rSubBindIndex);
    const ResMaterialAnim* m_pRes;
    const ResPerMaterialAnim* m_pMaterialAnims;
    int m_MaterialAnimCapacity;
    int m_ParamAnimCapacity;
    int m_TextureCapacity;
    int m_CurveCapacity;
    u16* m_pSubBindIndices;
    const nn::gfx::TextureView** m_ppTextureArray;
    u64* m_pTextureSlotArray;
};

static_assert(sizeof(MaterialAnimObj) == 0xa0, "Material animation object size");

class ShapeObj {
public:
    struct Impl;
    void ClearBlendWeights();
    size_t CalculateShapeBlockBufferSize(gfx::Device* pDevice) const;
    size_t CalculateDynamicVertexBufferSize(gfx::Device* pDevice) const;
    const gfx::Buffer* GetDynamicVertexBuffer(int vertexBufferIndex, int bufferIndex) const;
    bool IsDynamicVertexAttr(int attributeIndex) const;
    struct InitializeArgument {
        const ResShape* resource;
        int bufferCount;
        int viewCount;
        bool viewDependent;
        bool boundingEnabled;
        const void* userArea;
        size_t memorySize;
        size_t memoryAlignment;
        detail::WorkMemoryBlock blocks[8];
        void CalculateMemorySize();
    };
    // argument selects the shape; buffer supplies bufferSize bytes of working memory.
    bool Initialize(const InitializeArgument& argument, void* buffer, size_t bufferSize);
    const ResShape* GetResource() const { return m_pRes; }
    const Sphere* GetBounding() const { return m_pBounding; }
    /**
     * @brief Get the world-space bounding sphere for a level of detail.
     * @param lodIndex Mesh level-of-detail index selecting a local/world sphere pair.
     * @return World-space sphere, or nullptr when bounding storage is disabled.
     */
    const Sphere* GetBounding(int lodIndex) const {
        return (m_pBounding != nullptr) ? &m_pBounding[2 * lodIndex] : nullptr;
    }
    // device supplies GPU block requirements and owns their resources.
    size_t GetBlockBufferAlignment(gfx::Device* device) const;
    size_t CalculateBlockBufferSize(gfx::Device* device) const;
    // pool supplies size bytes starting at offset for the shape blocks.
    bool SetupBlockBuffer(gfx::Device* device, gfx::MemoryPool* pool, ptrdiff_t offset, size_t size);
    void CleanupBlockBuffer(gfx::Device* device);
    // skeleton supplies bone transforms; lodIndex selects the mesh bounds.
    void CalculateBounding(const SkeletonObj* skeleton, int lodIndex);
    // skeleton supplies bone transforms; lodIndex selects the mesh whose sub-mesh bounds are updated.
    void CalculateSubMeshBounding(const SkeletonObj* skeleton, int lodIndex);
    // viewIndex selects a camera, world supplies its transform, bufferIndex selects the GPU block.
    void CalculateShape(int viewIndex, const nn::util::Matrix4x3fType& rWorld, int bufferIndex);
    // bufferIndex selects the shape-animation destination buffer.
    void CalculateShapeAnimResult(int bufferIndex);
    /**
     * @brief Check whether shape-animation data is present and its calculation is enabled.
     * @return True when both required shape-animation flags are set.
     */
    bool IsShapeAnimCalculationEnabled() const { return (m_Flag & 12) == 12; }
    /**
     * @brief Enable shape-animation calculations for this shape.
     */
    void SetShapeAnimCalculationEnabled() { m_Flag |= 8; }
    /**
     * @brief Disable shape-animation calculations for this shape.
     */
    void SetShapeAnimCalculationDisabled() { m_Flag &= ~8; }
    const Aabb* GetSubMeshBoundingArray() const { return m_pSubMeshBoundingArray; }
    bool TestSubMeshIntersection(CullingContext* pContext, const ViewVolume& rViewVolume,
                                 int lodIndex) const;

    bool IsBlockBufferValid() const { return m_Flag & Flag_BlockBufferValid; }
    bool IsViewDependent() const { return m_ViewDependent; }

    BufferImpl* GetShapeBlock(int viewIndex, int bufferIndex)
    {
        if (!IsBlockBufferValid())
        {
            return nullptr;
        }
        if (IsViewDependent())
        {
            return (m_pShapeBlockArray != nullptr) ?
                       &m_pShapeBlockArray[viewIndex * m_BufferingCount + bufferIndex] :
                       nullptr;
        }
        return (m_pShapeBlockArray != nullptr) ? &m_pShapeBlockArray[bufferIndex] : nullptr;
    }

    const BufferImpl* GetShapeBlock(int viewIndex, int bufferIndex) const {
        if (!IsBlockBufferValid()) {
            return nullptr;
        }
        if (IsViewDependent()) {
            return (m_pShapeBlockArray != nullptr) ?
                       &m_pShapeBlockArray[viewIndex * m_BufferingCount + bufferIndex] :
                       nullptr;
        }
        return (m_pShapeBlockArray != nullptr) ? &m_pShapeBlockArray[bufferIndex] : nullptr;
    }

private:
    enum Flag { Flag_BlockBufferValid = 1 << 0 };

    const ResShape* m_pRes;
    u32 m_Flag;
    u8 _c;
    u8 m_ViewDependent;
    u8 m_ShapeBlockCount;
    u8 m_BufferingCount;
    u8 _10[0x20 - 0x10];
    BufferImpl* m_pShapeBlockArray;
    float* m_pBlendWeights;
    u32* m_pBlendWeightFlags;
    Sphere* m_pBounding;
    Aabb* m_pSubMeshBoundingArray;
    gfx::Buffer** m_ppDynamicVertexBuffers;
    const void* m_pUserArea;
    size_t m_UserAreaSize;
    u8 _60[0x70 - 0x60];
};

}  // namespace nn::g3d
