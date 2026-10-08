#pragma once

#include <nn/gfx/gfx_Buffer.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/g3d/g3d_World.h>
#include <nn/g3d/g3d_Resources.h>
#include <nn/types.h>
#include "nn/g3d/g3d_ResSkeleton.h"
#include "nn/util/util_MathTypes.h"

namespace nn::g3d {

struct LocalMtx {
    nn::Bit32 flag;
    nn::util::Vector3fType scale;
    nn::util::Matrix4x3fType mtx;
};

static_assert(sizeof(LocalMtx) == 0x60);

class SkeletonObj {
  public:
    /**
     * @brief Construct an empty skeleton object without allocated GPU or working storage.
     */
    SkeletonObj()
        : m_Res(nullptr), m_Flag(0), m_BufferingCount(0), m_Bones(nullptr), m_pLocalMtxArray(nullptr),
          m_WorldMtxArray(nullptr), m_pScaleArray(nullptr), m_MemoryPool(nullptr), m_MemoryPoolOffset(0),
          m_pMtxBlockArray(nullptr), m_MtxBlockSize(0), m_BoneCount(0), m_CallbackBoneIndex(0),
          m_Callback(nullptr), m_pLocalMtxBuffer(nullptr), m_pWorldMtxBuffer(nullptr), m_UserData(nullptr),
          m_WorkMemory(nullptr) {}
    struct InitializeArgument {
        const ResSkeleton* resource;
        int bufferCount;
        size_t memorySize;
        size_t memoryAlignment;
        detail::WorkMemoryBlock blocks[4];
        void CalculateMemorySize();
    };
    bool Initialize(const InitializeArgument& argument, void* buffer, size_t bufferSize);
    /**
     * @brief Access a bone resource by skeleton index.
     * @param index Bone index in the range [0, GetBoneCount()).
     * @return Pointer to the selected bone resource.
     */
    const ResBone* GetBone(int index) const { return &m_Bones[index]; }
    /** @brief Access the skeleton resource.
     * @return Resource selected at initialization, or nullptr before initialization.
     */
    const ResSkeleton* GetRes() const { return m_Res; }
    // name selects a bone; returns -1 when the skeleton has no bone with that name.
    int FindBoneIndex(const char* name) const;

    /** @brief Access the calculated world transforms.
     * @return Array containing one matrix per bone, or nullptr before initialization.
     */
    const nn::util::Matrix4x3fType* GetWorldMtxArray() const { return m_WorldMtxArray; }
    /** @brief Access writable world transforms.
     * @return Array containing one matrix per bone, or nullptr before initialization.
     */
    nn::util::Matrix4x3fType* GetWorldMtxArray() { return m_WorldMtxArray; }
    /** @brief Query the number of bones in this skeleton.
     * @return Number of entries in the bone and transform arrays.
     */
    int GetBoneCount() const { return m_BoneCount; }
    /**
     * @brief Access the local transform of one bone.
     * @param index Bone index in the range [0, GetBoneCount()).
     * @return Pointer to the selected local transform.
     */
    const LocalMtx* GetLocalMtx(int index) const { return &m_pLocalMtxArray[index]; }
    /** @brief Access writable local transforms.
     * @return Array containing one local transform per bone, or nullptr before initialization.
     */
    LocalMtx* GetLocalMtxArray() { return m_pLocalMtxArray; }

    /**
     * @brief Access a buffered GPU matrix block.
     * @param bufferIndex Index below the buffering count selected at initialization.
     * @return Selected buffer, or nullptr when no buffer array is assigned.
     */
    const gfx::Buffer* GetMtxBlock(int bufferIndex) const {
        return (m_pMtxBlockArray != nullptr) ? &m_pMtxBlockArray[bufferIndex] : nullptr;
    }

    /** @brief Check whether matrix-buffer setup has completed.
     * @return True after setup and false after cleanup.
     */
    bool IsBlockBufferValid() const { return (m_Flag & 1) != 0; }
    size_t GetBlockBufferAlignment(gfx::Device* device) const;
    size_t CalculateBlockBufferSize(gfx::Device* device) const;
    bool SetupBlockBuffer(gfx::Device* device, gfx::MemoryPool* pool, ptrdiff_t offset, size_t size);
    void CleanupBlockBuffer(gfx::Device* device);
    // world is the root model-to-world transform.
    void CalculateWorldMtx(const nn::util::Matrix4x3fType& world);
    void CalculateSkeleton(int bufferIndex);
    // output receives the billboard transform for boneIndex and view; world includes its world transform.
    void CalculateBillboardMtx(nn::util::Matrix4x3fType* output, const nn::util::Matrix4x3fType& view,
                               int boneIndex, bool world) const;

    /** @brief Query the payload size of one GPU matrix block.
     * @return Matrix payload size in bytes, excluding inter-buffer alignment padding.
     */
    size_t GetMtxBlockSize() const { return m_MtxBlockSize; }

    /**
     * @brief Set the callback used while calculating bone world transforms.
     * @param pCallback Callback object that outlives its use, or nullptr to disable callbacks.
     */
    void SetCalculateWorldCallback(ICalculateWorldCallback* pCallback) {
        m_Callback = pCallback;
        m_CallbackBoneIndex = m_BoneCount == 0 ? -1 : 0;
    }

  private:
    void ClearLocalMtx();
    void SetupBlockBufferImpl(gfx::Device* pDevice, gfx::MemoryPool* pPool, ptrdiff_t offset, size_t size);
    const ResSkeleton* m_Res;
    u16 m_Flag;
    u8 m_BufferingCount;
    u8 _b[5];
    const ResBone* m_Bones;
    LocalMtx* m_pLocalMtxArray;
    nn::util::Matrix4x3fType* m_WorldMtxArray;
    nn::util::Vector3fType* m_pScaleArray;
    gfx::MemoryPool* m_MemoryPool;
    ptrdiff_t m_MemoryPoolOffset;
    gfx::Buffer* m_pMtxBlockArray;
    size_t m_MtxBlockSize;
    u16 m_BoneCount;
    s16 m_CallbackBoneIndex;
    ICalculateWorldCallback* m_Callback;
    LocalMtx* m_pLocalMtxBuffer;
    nn::util::Matrix4x3fType* m_pWorldMtxBuffer;
    void* m_UserData;
    void* m_WorkMemory;
};

static_assert(sizeof(SkeletonObj) == 0x80);

} // namespace nn::g3d
