#pragma once

#include <basis/seadTypes.h>
#include <nvn/nvn.h>
#include <nvn/nvn_FuncPtrInline.h>
#include <prim/seadBitFlag.h>

#include "common/aglGPUMemAddr.h"

namespace sead {
class Heap;
}

namespace agl {

class DrawContext;
class ShaderLocation;

class UniformBlock {
public:
    enum Type {
        cType_Bool = 0,
        cType_Int = 1,
        cType_UInt = 2,
        cType_Float = 3,
        cType_Vec2 = 4,
        cType_Vec3 = 5,
        cType_Vec4 = 6,
        cType_IVec4 = 7,
        cType_Mat2x2 = 8,
        cType_Mat2x3 = 9,
        cType_Mat2x4 = 10,
        cType_Mat3x2 = 11,
        cType_Mat3x3 = 12,
        cType_Mat3x4 = 13,
        cType_Mat4x2 = 14,
        cType_Mat4x3 = 15,
        cType_Mat4x4 = 16,
        cType_Struct = 17,
        cType_Num = 18,
    };

    enum Flag {
        cFlag_OwnHeader = 1 << 0,
        cFlag_OwnBuffer = 1 << 1,
        cFlag_NvnBufferInitialized = 1 << 2,
    };

    struct Member {
        u32 mNum;
        u16 mOffset;
        u16 mStride;
        u8 mType;
    };
    static_assert(sizeof(Member) == 0xc);

    struct Header {
        Member* mMembers;
        u16 mMemberNum;
        u16 mDeclaredNum;
    };
    static_assert(sizeof(Header) == 0x10);

    UniformBlock();
    virtual ~UniformBlock();

    void destroy();
    void startDeclare(s32 memberNum, sead::Heap* pHeap);
    void declare(const UniformBlock& rOther);
    u64 calcRequiredBufferSize(s32 bufferNum, s32 blockNum) const;
    void create(sead::Heap* pHeap, s32 bufferNum, s32 blockNum);
    void setBuffer(GPUMemVoidAddr buffer, s32 bufferNum, s32 blockNum);
    void createWithBuffer(GPUMemVoidAddr buffer, s32 bufferNum, s32 blockNum);
    void createWithoutBuffer();
    void createDirect(GPUMemVoidAddr buffer, u64 size);
    void copyAllTo(UniformBlock* pDst, s32 bufferIndex, s32 blockIndex) const;
    void copyPartialTo(UniformBlock* pDst, s32 srcFirstMember, s32 srcLastMember,
                       s32 dstMember, s32 unused, s32 bufferIndex, s32 blockIndex) const;
    void fill(u32 value, s32 blockIndex);
    static void writeMemory(u32* pDst, const u32* pSrc, s32 count, s32 dstStride, s32 size,
                            const void* pUnused);
    void dcbz(s32 blockIndex) const;
    void dcbz(u32 unused, s32 blockIndex) const;

    virtual void invalidateGPUCache(DrawContext* pDrawContext, s32 blockIndex) const {}

    u32 getBlockSize() const { return mBlockSize; }
    u32 getAlignedBlockSize() const { return mAlignedBlockSize; }
    u32 getBufferSize() const { return mBufferSize; }
    s32 getBufferNum() const { return mBufferNum; }
    void* getCurrentBuffer() const { return mCurrentBuffer; }
    GPUMemVoidAddr getBuffer() const { return mBuffer; }
    const NVNbuffer* getNvnBuffer() const { return &mNvnBuffer; }
    u32 getCurrentBlockOffset(s32 blockIndex) const
    {
        return (mCurrentBufferIndex * mBlockNum + blockIndex) * mAlignedBlockSize;
    }

    void declare(Type type, s32 num) { declare_(type, num, 0, 1); }
    void declareStruct(s32 num, u64 size, u64 alignment)
    {
        declare_(cType_Struct, num, size, alignment);
    }
    void setDataStruct(s32 memberIndex, const void* pData, s32 arrayIndex, s32 count,
                       u32 structSize) const
    {
        setDataStruct_(mCurrentBuffer, memberIndex, pData, arrayIndex, count, structSize);
    }
    void setData(s32 memberIndex, const void* pData, s32 arrayIndex, s32 count) const
    {
        setData_(mCurrentBuffer, memberIndex, pData, arrayIndex, count);
    }
    void setCurrentBufferIndex(s32 bufferIndex)
    {
        mCurrentBufferIndex = bufferIndex;
        mCurrentBuffer = getBlockPtr_(-1, 0);
    }
    bool setUniform(DrawContext* pDrawContext, const u64& rAddress, const ShaderLocation& rLocation,
                    u32 offset, u64 size) const;  // inline, defined below
    void activate(DrawContext* pDrawContext, const ShaderLocation& rLocation) const
    {
        u64 address = nvnBufferGetAddress(&mNvnBuffer) +
                      mCurrentBufferIndex * mBlockNum * mAlignedBlockSize;
        setUniform(pDrawContext, address, rLocation, 0, mBlockSize);
    }
    void flushCurrentBuffer() const
    {
        u32 offset = mCurrentBufferIndex * mBlockNum * mAlignedBlockSize;
        GPUMemVoidAddr address;
        address = GPUMemVoidAddr(mBuffer, offset);
        address.flushCPUCache(mBlockSize);
    }

protected:
    void declare_(Type type, s32 num, u64 size, u64 alignment);
    void setData_(void* pBuffer, s32 memberIndex, const void* pData, s32 arrayIndex,
                  s32 count) const;
    void setDataStruct_(void* pBuffer, s32 memberIndex, const void* pData, s32 arrayIndex,
                        s32 count, u32 structSize) const;

    virtual u8 getStrideArray_(u8 type) const;
    virtual u32 getBlockAlignment_() const { return 0x100; }
    virtual u32 getBlockSizeMax_() const { return 0x10000; }
    virtual void verifyStructSize_(u32 size) const {}
    virtual void bindBufferNVN_(NVNcommandBuffer* pCommandBuffer, NVNshaderStage stage, s32 slot,
                                u64 address, u64 size) const
    {
        nvnCommandBufferBindUniformBuffer(pCommandBuffer, stage, slot, address, size);
    }

private:
    s32 calcBufferSize_(s32 bufferNum, s32 blockNum) const
    {
        return mAlignedBlockSize * (bufferNum * blockNum - 1) +
               ((mBlockSize + getBlockAlignment_() - 1) & -getBlockAlignment_());
    }

    void* getBlockPtr_(s32 bufferIndex, s32 blockIndex) const
    {
        if (bufferIndex == -1)
        {
            bufferIndex = mCurrentBufferIndex;
        }
        return GPUMemVoidAddr(mBuffer, (bufferIndex * mBlockNum + blockIndex) * mAlignedBlockSize)
            .getPtr();
    }

    Header* mHeader;
    void* mCurrentBuffer;
    u32 mBlockSize;
    u32 mAlignedBlockSize;
    u32 mBufferSize;
    GPUMemVoidAddr mBuffer;
    u8 mBufferNum;
    u8 mCurrentBufferIndex;
    sead::BitFlag8 mFlags;
    u16 mBlockNum;
    alignas(8) NVNbuffer mNvnBuffer;
};
static_assert(sizeof(UniformBlock) == 0x78);

class ShaderStorageBlock : public UniformBlock {};

}  // namespace agl

#include "common/aglDrawContext.h"
#include "common/aglShaderLocation.h"
#include "driver/aglNVNMgr.h"

namespace agl {

inline bool UniformBlock::setUniform(DrawContext* pDrawContext, const u64& rAddress,
                                     const ShaderLocation& rLocation, u32 offset, u64 size) const
{
    if (!rLocation.isValid())
        return false;

    u64 address = rAddress + offset;
    NVNcommandBuffer* commandBuffer = pDrawContext->getNvnCommandBuffer();
    for (s32 i = 0; i < cShaderType_Num; i++)
    {
        ShaderType type = static_cast<ShaderType>(i);
        if (rLocation.getLocation(type) != -1)
        {
            bindBufferNVN_(commandBuffer, driver::NVNMgr::getNVNshaderStage(type),
                           rLocation.getLocation(type), address, size);
        }
    }
    return true;
}

}  // namespace agl
