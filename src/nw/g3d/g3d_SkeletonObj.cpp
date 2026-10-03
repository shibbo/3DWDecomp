#include <nn/g3d/g3d_SkeletonObj.h>
#include <nn/g3d/g3d_TransformImpl.h>
#include <nn/gfx/gfx_MemoryPool.h>
#include <nn/util/util_MatrixApi.h>
#include <nn/util/util_VectorApi.h>
#include <cstring>
#include <new>

namespace nn::g3d {
namespace {
using Matrix = util::Matrix4x3fType;
using PackedMatrix = util::FloatColumnMajor4x3;
/**
 * @brief Multiply an inverse-bind matrix by a bone's world transform.
 * @param rInverseBind Packed inverse model matrix for one smooth matrix entry.
 * @param world Current world transform of the corresponding bone.
 * @return Composed skinning matrix with padded rows.
 */
inline Matrix MultiplyInverseBind(const PackedMatrix& rInverseBind, Matrix world) {
    float32x4_t a0 = vld1q_f32(rInverseBind.m[0]);
    float32x4_t a1 = vld1q_f32(rInverseBind.m[1]);
    float32x4_t a2 = vld1q_f32(rInverseBind.m[2]);
    float32x4_t b0 = world._m.val[0], b1 = world._m.val[1];
    float32x4_t b2 = world._m.val[2], b3 = world._m.val[3];
    Matrix result;
    float32x4_t r0 = vmulq_laneq_f32(b0, a0, 0);
    r0 = vfmaq_laneq_f32(r0, b1, a1, 0);
    float32x4_t r1 = vmulq_laneq_f32(b0, a0, 1);
    r1 = vfmaq_laneq_f32(r1, b1, a1, 1);
    float32x4_t r2 = vmulq_laneq_f32(b0, a0, 2);
    r2 = vfmaq_laneq_f32(r2, b1, a1, 2);
    float32x4_t r3 = vmulq_laneq_f32(b0, a0, 3);
    r3 = vfmaq_laneq_f32(r3, b1, a1, 3);
    r3 = vfmaq_laneq_f32(r3, b2, a2, 3);
    result._m.val[0] = vfmaq_laneq_f32(r0, b2, a2, 0);
    result._m.val[1] = vfmaq_laneq_f32(r1, b2, a2, 1);
    result._m.val[2] = vfmaq_laneq_f32(r2, b2, a2, 2);
    result._m.val[3] = vaddq_f32(b3, r3);
    return result;
}
/**
 * @brief Store padded matrix rows as three packed columns for the GPU.
 * @param pOutput Destination with space for one packed matrix.
 * @param rMatrix Matrix whose three non-padding components are stored.
 */
inline void StorePackedMatrix(PackedMatrix* pOutput, const Matrix& rMatrix) {
    util::MatrixStore(pOutput, rMatrix);
}
/**
 * @brief Restore one bone's local flags, scale and rotation/translation matrix.
 * @tparam euler Whether the resource stores Euler angles instead of a quaternion.
 * @param pOutput Writable local transform belonging to this bone.
 * @param rBone Resource bone supplying default transform components.
 */
template <bool euler>
inline void RestoreLocalTransform(LocalMtx* pOutput, const ResBone& rBone) {
    pOutput->flag = rBone.ToData().flag & (ResBone::Mask_Transform | ResBone::Mask_Billboard);
    util::VectorLoad(&pOutput->scale, rBone.GetScale());
    Matrix rotation;
    if (euler) {
        util::Vector3fType angles;
        util::VectorLoad(&angles, rBone.GetRotateEuler());
        rotation = detail::MakeEulerRotation(angles._v);
    } else {
        rotation = detail::MakeQuaternionRotation(vld1q_f32(rBone.GetRotateQuat().v));
    }
    util::Vector3fType translation;
    util::VectorLoad(&translation, rBone.GetTranslate());
    rotation._m.val[3] = translation._v;
    pOutput->mtx = rotation;
}
} // namespace
/** @brief Restore every local transform from the skeleton's resource bone defaults. */
void SkeletonObj::ClearLocalMtx() {
    LocalMtx* pLocal = m_pLocalMtxArray;
    size_t count = m_Res->GetBoneCount();
    if ((m_Flag & ResBone::Mask_Rot) != 0) {
        for (size_t i = 0; i < count; ++i) {
            RestoreLocalTransform<true>(&pLocal[i], m_Res->ToData().pBoneArray.Get()[i]);
        }
    } else {
        for (size_t i = 0; i < count; ++i) {
            RestoreLocalTransform<false>(&pLocal[i], m_Res->ToData().pBoneArray.Get()[i]);
        }
    }
}
/** @brief Calculate workspace offsets for world matrices, local matrices, scales and GPU buffers. */
void SkeletonObj::InitializeArgument::CalculateMemorySize() {
    auto scaleMode = resource->GetScaleMode();
    int boneCount = resource->GetBoneCount();
    int buffers = bufferCount;
    for (int i = 0; i < 4; ++i) {
        blocks[i].Initialize(0);
    }
    blocks[0].size = boneCount * sizeof(util::Matrix4x3fType);
    blocks[0].alignment = 16;
    blocks[1].size = boneCount * sizeof(LocalMtx);
    blocks[1].alignment = 16;
    if (scaleMode == 0x300) {
        blocks[2].Initialize(boneCount * sizeof(util::Vector3fType), 16);
    }
    blocks[3].size = buffers * sizeof(gfx::Buffer);
    memorySize = 0;
    memoryAlignment = 8;
    for (int i = 0; i < 4; ++i) {
        detail::AppendWorkspaceBlock(blocks[i], memorySize, memoryAlignment, blocks[i].alignment);
    }
}
/**
 * @brief Initialize skeleton storage and restore the resource's local transforms.
 * @param rArgument Skeleton resource, buffering count and calculated workspace layout.
 * @param pBuffer Caller-owned workspace aligned to the calculated requirement.
 * @param bufferSize Available workspace bytes; must cover the calculated requirement.
 * @return True if the calculated workspace fits the supplied buffer.
 */
bool SkeletonObj::Initialize(const InitializeArgument& rArgument, void* pBuffer, size_t bufferSize) {
    if (rArgument.memoryAlignment == 0) {
        return false;
    }
    if (rArgument.memorySize > bufferSize) {
        return false;
    }
    const ResSkeleton* pResource = rArgument.resource;
    m_Res = pResource;
    m_Flag = pResource->ToData().flag;
    m_BufferingCount = rArgument.bufferCount;
    m_Bones = pResource->ToData().pBoneArray.Get();
    m_pLocalMtxArray = rArgument.blocks[1].GetPointer<LocalMtx>(pBuffer);
    m_pLocalMtxBuffer = m_pLocalMtxArray;
    m_WorldMtxArray = rArgument.blocks[0].GetPointer<util::Matrix4x3fType>(pBuffer);
    m_pWorldMtxBuffer = m_WorldMtxArray;
    m_pScaleArray = rArgument.blocks[2].GetPointer<util::Vector3fType>(pBuffer);
    m_pMtxBlockArray = rArgument.blocks[3].GetPointer<gfx::Buffer>(pBuffer);
    m_BoneCount = pResource->GetBoneCount();
    m_CallbackBoneIndex = -1;
    m_Callback = nullptr;
    m_MtxBlockSize = 0;
    m_UserData = nullptr;
    m_WorkMemory = pBuffer;
    m_MemoryPool = nullptr;
    m_MemoryPoolOffset = 0;
    ClearLocalMtx();
    return true;
}
/**
 * @brief Query the graphics backend's alignment for skeleton matrix blocks.
 * @param pDevice Initialized graphics device providing buffer requirements.
 * @return Required matrix-block alignment in bytes.
 */
size_t SkeletonObj::GetBlockBufferAlignment(gfx::Device* pDevice) const {
    gfx::BufferInfo info;
    std::memset(&info, 0, sizeof(info));
    info.SetDefault();
    info.SetSize(m_Res->GetMtxCount() * sizeof(util::FloatColumnMajor4x3));
    info.SetGpuAccessFlags(0x50);
    return BufferImpl::GetBufferAlignment(pDevice, info);
}
/**
 * @brief Calculate aligned GPU storage for all buffered skeleton matrix copies.
 * @param pDevice Initialized graphics device providing buffer alignment.
 * @return Total matrix-buffer storage requirement in bytes.
 */
size_t SkeletonObj::CalculateBlockBufferSize(gfx::Device* pDevice) const {
    size_t size = m_Res->GetMtxCount() * sizeof(util::FloatColumnMajor4x3);
    size_t alignment = GetBlockBufferAlignment(pDevice);
    return ((size + alignment - 1) & -alignment) * m_BufferingCount;
}
/**
 * @brief Create each GPU matrix buffer within a validated memory-pool region.
 * @param pDevice Device owning the new buffers.
 * @param pPool Memory pool providing storage for the matrix blocks.
 * @param offset Aligned starting byte offset into the pool.
 * @param size Unused; SetupBlockBuffer has already checked the available storage.
 */
void SkeletonObj::SetupBlockBufferImpl(gfx::Device* pDevice, gfx::MemoryPool* pPool, ptrdiff_t offset,
                                       size_t size) {
    m_MtxBlockSize = m_Res->GetMtxCount() * sizeof(util::FloatColumnMajor4x3);
    for (int i = 0; i < m_BufferingCount; ++i) {
        gfx::BufferInfo info;
        std::memset(&info, 0, sizeof(info));
        info.SetDefault();
        info.SetSize(m_MtxBlockSize);
        info.SetGpuAccessFlags(0x50);
        gfx::Buffer* pBuffer = new (&m_pMtxBlockArray[i]) gfx::Buffer;
        pBuffer->Initialize(pDevice, info, pPool, offset, m_MtxBlockSize);
        gfx::util::SetBufferDebugLabel(pBuffer, "g3d_SkeletonUniformBlock");
        size_t blockSize = m_MtxBlockSize;
        size_t alignment = GetBlockBufferAlignment(pDevice);
        offset += (blockSize + alignment - 1) & -alignment;
    }
    m_Flag |= 1;
}
/**
 * @brief Validate GPU storage and initialize buffered matrix blocks when needed.
 * @param pDevice Device owning the buffers and providing alignment requirements.
 * @param pPool Pool containing the supplied region.
 * @param offset Aligned start of the region in bytes.
 * @param size Available bytes in the region.
 * @return False if storage is insufficient; true after successful setup or when no storage is needed.
 */
bool SkeletonObj::SetupBlockBuffer(gfx::Device* pDevice, gfx::MemoryPool* pPool, ptrdiff_t offset,
                                   size_t size) {
    size_t required = CalculateBlockBufferSize(pDevice);
    if (required > size) {
        return false;
    }
    if (required != 0) {
        m_MemoryPool = pPool;
        m_MemoryPoolOffset = offset;
        SetupBlockBufferImpl(pDevice, pPool, offset, size);
    }
    return true;
}
/**
 * @brief Destroy matrix buffers and clear the associated pool state.
 * @param pDevice Device owning the initialized GPU buffers.
 */
void SkeletonObj::CleanupBlockBuffer(gfx::Device* pDevice) {
    for (int i = 0; i < m_BufferingCount; ++i) {
        BufferImpl* pBuffer = &m_pMtxBlockArray[i];
        pBuffer->Finalize(pDevice);
        pBuffer->~BufferImpl();
    }
    m_Flag &= ~1;
    m_MtxBlockSize = 0;
    m_MemoryPool = nullptr;
    m_MemoryPoolOffset = 0;
}
/**
 * @brief Upload smooth skinning matrices and rigid world matrices to a buffered GPU block.
 * @param bufferIndex Buffered matrix-block index below the initialized buffering count.
 */
void SkeletonObj::CalculateSkeleton(int bufferIndex) {
    if (m_MtxBlockSize == 0) {
        return;
    }
    const short* pBoneIndices = m_Res->ToData().pMtxToBoneTable.Get();
    const PackedMatrix* pInverseMatrices = m_Res->ToData().pInvModelMatrixArray.Get();
    auto* pOutput = static_cast<PackedMatrix*>(m_pMtxBlockArray[bufferIndex].Map());
    unsigned count = m_Res->GetSmoothMtxCount();
    unsigned i = 0;
    for (; i < count; ++i) {
        Matrix matrix = MultiplyInverseBind(pInverseMatrices[i], m_WorldMtxArray[pBoneIndices[i]]);
        StorePackedMatrix(&pOutput[i], matrix);
    }
    count = m_Res->GetMtxCount();
    for (; i < count; ++i) {
        int boneIndex = pBoneIndices[i];
        StorePackedMatrix(&pOutput[i], m_WorldMtxArray[boneIndex]);
    }
    m_pMtxBlockArray[bufferIndex].FlushMappedRange(0, m_MtxBlockSize);
    m_pMtxBlockArray[bufferIndex].Unmap();
}

} // namespace nn::g3d
