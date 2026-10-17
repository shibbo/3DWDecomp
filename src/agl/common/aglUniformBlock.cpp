#include "common/aglUniformBlock.h"

#include <cstring>
#include <heap/seadHeap.h>
#include <nvn/nvn_FuncPtrInline.h>

#include "common/aglGPUMemBlock.h"
#include "driver/aglNVNMgr.h"

namespace agl {

namespace {

struct TypeInfo {
    u8 mSize;
    u8 mAlignment;
    u8 mArrayStride;
    u8 _3;
};

const TypeInfo cTypeInfo[UniformBlock::cType_Num] = {
    {1, 1, 4, 0},    {1, 1, 4, 0},    {1, 1, 4, 0},    {1, 1, 4, 0},    {2, 2, 4, 0},
    {3, 4, 4, 0},    {4, 4, 4, 0},    {4, 4, 4, 0},    {8, 8, 8, 0},    {8, 8, 8, 0},
    {8, 8, 8, 0},    {12, 12, 12, 0}, {12, 12, 12, 0}, {12, 12, 12, 0}, {16, 16, 16, 0},
    {16, 16, 16, 0}, {16, 16, 16, 0}, {1, 1, 4, 0},
};

}  // namespace

/**
 * Constructs an empty uniform block.
 */
UniformBlock::UniformBlock()
    : mHeader(nullptr), mCurrentBuffer(nullptr), mBlockSize(0), mAlignedBlockSize(0),
      mBufferSize(0), mBufferNum(1), mCurrentBufferIndex(0), mFlags(0), mBlockNum(1)
{
}

/**
 * Destroys the uniform block.
 */
UniformBlock::~UniformBlock()
{
    destroy();
}

/**
 * Releases the NVN buffer, the member declarations and the buffer memory.
 */
void UniformBlock::destroy()
{
    if (mFlags.isOn(cFlag_NvnBufferInitialized))
    {
        nvnBufferFinalize(&mNvnBuffer);
        mFlags.reset(cFlag_NvnBufferInitialized);
    }

    mCurrentBuffer = nullptr;
    mBlockSize = 0;
    mAlignedBlockSize = 0;
    mBufferSize = 0;

    if (mFlags.isOn(cFlag_OwnHeader))
    {
        delete[] mHeader->mMembers;
        delete mHeader;
        mFlags.reset(cFlag_OwnHeader);
    }

    mHeader = nullptr;

    if (mFlags.isOn(cFlag_OwnBuffer))
    {
        if (mBuffer.isValid())
        {
            mBuffer.deleteGPUMemBlock();
            mBuffer.invalidate();
        }

        mFlags.reset(cFlag_OwnBuffer);
    }
}

/**
 * Starts declaring members.
 * @param memberNum number of members that will be declared
 * @param pHeap heap to allocate the declarations from
 */
void UniformBlock::startDeclare(s32 memberNum, sead::Heap* pHeap)
{
    mHeader = new (pHeap, 8) Header;
    mHeader->mMembers = new (pHeap, 8) Member[memberNum];
    mHeader->mMemberNum = memberNum;
    mHeader->mDeclaredNum = 0;
    mFlags.set(cFlag_OwnHeader);
    mBlockSize = 0;
    mAlignedBlockSize = 0;
}

/**
 * Gets the array stride of a member type in words.
 * @param type member type
 * @return array stride in words
 */
u8 UniformBlock::getStrideArray_(u8 type) const
{
    return cTypeInfo[type].mArrayStride;
}

/**
 * Declares the next member.
 * @param type member type
 * @param num array length
 * @param size struct size in bytes (structs only)
 * @param alignment struct alignment in bytes (structs only)
 */
void UniformBlock::declare_(Type type, s32 num, u64 size, u64 alignment)
{
    Member& rMember = mHeader->mMembers[mHeader->mDeclaredNum];
    rMember.mType = type;
    rMember.mNum = num;

    u32 align;

    if (type == cType_Struct)
    {
        rMember.mStride = size / 4;
        align = alignment / 4;
    }
    else if (num == 1)
    {
        align = cTypeInfo[rMember.mType].mAlignment;
        rMember.mStride = cTypeInfo[rMember.mType].mSize;
    }
    else
    {
        align = getStrideArray_(type);
        rMember.mStride = align;
    }

    mBlockSize = (mBlockSize + align * 4 - 1) & -(align * 4);
    rMember.mOffset = mBlockSize;
    mBlockSize += rMember.mStride * rMember.mNum * 4;

    mHeader->mDeclaredNum++;
    u32 blockSize = mBlockSize;

    if (mHeader->mDeclaredNum == mHeader->mMemberNum)
    {
        mBlockSize = (blockSize + 0x1f) & ~0x1f;
        mAlignedBlockSize = (mBlockSize + getBlockAlignment_() - 1) & -getBlockAlignment_();
    }
}

/**
 * Shares the member declarations of another uniform block.
 * @param rOther uniform block to take the declarations from
 */
void UniformBlock::declare(const UniformBlock& rOther)
{
    mHeader = rOther.mHeader;
    mBlockSize = rOther.mBlockSize;
    mAlignedBlockSize = rOther.mAlignedBlockSize;
    mFlags.reset(cFlag_OwnHeader);
}

/**
 * Calculates the buffer size needed for a number of blocks.
 * @param bufferNum number of buffers
 * @param blockNum number of blocks per buffer
 * @return required buffer size in bytes
 */
u64 UniformBlock::calcRequiredBufferSize(s32 bufferNum, s32 blockNum) const
{
    u32 blocks = mAlignedBlockSize * (bufferNum * blockNum - 1);
    u64 last = (mBlockSize + getBlockAlignment_() - 1) & -getBlockAlignment_();
    return last + blocks;
}

/**
 * Allocates the buffer memory and creates the NVN buffer.
 * @param pHeap heap to allocate from
 * @param bufferNum number of buffers
 * @param blockNum number of blocks per buffer
 */
void UniformBlock::create(sead::Heap* pHeap, s32 bufferNum, s32 blockNum)
{
    u64 size = calcBufferSize_(bufferNum, blockNum);
    u32 alignment = getBlockAlignment_();
    auto* pBlock = new (pHeap, 8) GPUMemBlockU8;
    pBlock->allocBuffer_(size, pHeap, alignment, MemoryAttribute::_01);
    GPUMemVoidAddr buffer(*pBlock, 0);
    mFlags.set(cFlag_OwnBuffer);
    setBuffer(buffer, bufferNum, blockNum);
}

/**
 * Sets the buffer memory and creates the NVN buffer.
 * @param buffer buffer memory
 * @param bufferNum number of buffers
 * @param blockNum number of blocks per buffer
 */
void UniformBlock::setBuffer(GPUMemVoidAddr buffer, s32 bufferNum, s32 blockNum)
{
    mBufferNum = bufferNum;
    mBuffer = GPUMemVoidAddr(buffer, 0);
    mBlockNum = blockNum;

    if (mHeader != nullptr)
    {
        mBufferSize = calcBufferSize_(bufferNum, blockNum);
    }

    if (mBuffer.isValid())
    {
        if (mFlags.isOn(cFlag_NvnBufferInitialized))
        {
            nvnBufferFinalize(&mNvnBuffer);
        }

        NVNbufferBuilder builder;
        nvnBufferBuilderSetDevice(&builder, driver::NVNMgr::instance()->getNvnDevice());
        nvnBufferBuilderSetDefaults(&builder);
        nvnBufferBuilderSetStorage(&builder, mBuffer.getMemoryPool()->getDriverPool(),
                                   mBuffer.getByteOffset(), mBufferSize);
        nvnBufferInitialize(&mNvnBuffer, &builder);
        mFlags.set(cFlag_NvnBufferInitialized);
    }

    mCurrentBufferIndex = 0;
    mCurrentBuffer = getBuffer().getPtr();
}

/**
 * Uses caller-provided buffer memory.
 * @param buffer buffer memory
 * @param bufferNum number of buffers
 * @param blockNum number of blocks per buffer
 */
void UniformBlock::createWithBuffer(GPUMemVoidAddr buffer, s32 bufferNum, s32 blockNum)
{
    setBuffer(buffer, bufferNum, blockNum);
}

/**
 * Creates the block without buffer memory.
 */
void UniformBlock::createWithoutBuffer()
{
    GPUMemVoidAddr buffer;
    setBuffer(buffer, 1, 1);
}

/**
 * Uses caller-provided buffer memory of a given size without member declarations.
 * @param buffer buffer memory
 * @param size size of the block in bytes
 */
void UniformBlock::createDirect(GPUMemVoidAddr buffer, u64 size)
{
    mBlockSize = size;
    mBufferSize = size;
    setBuffer(buffer, 1, 1);
}

/**
 * Copies a whole block to another uniform block.
 * @param pDst destination uniform block
 * @param bufferIndex buffer index, or -1 for the current buffer
 * @param blockIndex block index inside the buffer
 */
void UniformBlock::copyAllTo(UniformBlock* pDst, s32 bufferIndex, s32 blockIndex) const
{
    void* pSrcPtr = getBlockPtr_(bufferIndex, blockIndex);
    void* pDstPtr = pDst->getBlockPtr_(bufferIndex, blockIndex);
    std::memcpy(pDstPtr, pSrcPtr, mBlockSize);
}

/**
 * Copies a range of members to another uniform block.
 * @param pDst destination uniform block
 * @param srcFirstMember first source member
 * @param srcLastMember last source member
 * @param dstMember destination member
 * @param unused unused
 * @param bufferIndex buffer index, or -1 for the current buffer
 * @param blockIndex block index inside the buffer
 */
void UniformBlock::copyPartialTo(UniformBlock* pDst, s32 srcFirstMember, s32 srcLastMember,
                                 s32 dstMember, s32 unused, s32 bufferIndex,
                                 s32 blockIndex) const
{
    u8* pSrcBase = static_cast<u8*>(getBlockPtr_(bufferIndex, blockIndex));
    const Member* pMembers = mHeader->mMembers;
    const u8* pSrcBegin = pSrcBase + pMembers[srcFirstMember].mOffset;
    const Member& rLast = pMembers[srcLastMember];
    const u8* pSrcEnd = pSrcBase + rLast.mOffset + rLast.mStride * rLast.mNum * 4;
    s32 size = pSrcEnd - pSrcBegin;

    u8* pDstBase = static_cast<u8*>(pDst->getBlockPtr_(bufferIndex, blockIndex));
    std::memcpy(pDstBase + pDst->mHeader->mMembers[dstMember].mOffset, pSrcBegin, size);
}

/**
 * Fills a block of the current buffer with a value.
 * @param value value to write to every word
 * @param blockIndex block index
 */
void UniformBlock::fill(u32 value, s32 blockIndex)
{
    u32 num = mBlockSize / 4;
    u32* pDst = reinterpret_cast<u32*>(static_cast<u8*>(mCurrentBuffer) +
                                       mAlignedBlockSize * blockIndex);
    for (u32 i = 0; i < num; i++)
    {
        pDst[i] = value;
    }
}

/**
 * Copies words into strided memory.
 * @param pDst destination
 * @param pSrc source
 * @param count number of elements
 * @param dstStride destination stride in words
 * @param size element size in words
 * @param pUnused unused
 */
void UniformBlock::writeMemory(u32* pDst, const u32* pSrc, s32 count, s32 dstStride, s32 size,
                               const void* pUnused)
{
    for (s32 i = 0; i < count; i++)
    {
        for (s32 j = 0; j < size; j++)
        {
            pDst[j] = *pSrc++;
        }

        pDst += dstStride;
    }
}

/**
 * Writes array elements of a member.
 * @param pBuffer block memory
 * @param memberIndex member index
 * @param pData source data
 * @param arrayIndex first array element
 * @param count number of array elements
 */
void UniformBlock::setData_(void* pBuffer, s32 memberIndex, const void* pData, s32 arrayIndex,
                            s32 count) const
{
    const Member& rMember = mHeader->mMembers[memberIndex];
    u8 type = rMember.mType;
    u8 stride = getStrideArray_(type);
    u32* pDst = reinterpret_cast<u32*>(static_cast<u8*>(pBuffer) +
                                       (rMember.mOffset + stride * arrayIndex * sizeof(u32)));
    writeMemory(pDst, static_cast<const u32*>(pData), count, stride, cTypeInfo[type].mSize,
                nullptr);
}

/**
 * Writes array elements of a struct member.
 * @param pBuffer block memory
 * @param memberIndex member index
 * @param pData source data
 * @param arrayIndex first array element
 * @param count number of array elements
 * @param structSize size of one struct in bytes
 */
void UniformBlock::setDataStruct_(void* pBuffer, s32 memberIndex, const void* pData,
                                  s32 arrayIndex, s32 count, u32 structSize) const
{
    verifyStructSize_(structSize);

    s32 words = structSize / 4;
    const Member& rMember = mHeader->mMembers[memberIndex];
    writeMemory(reinterpret_cast<u32*>(static_cast<u8*>(pBuffer) +
                                       rMember.mOffset) + words * arrayIndex,
                static_cast<const u32*>(pData), count, words, words, nullptr);
}

/**
 * Clears a block of the current buffer.
 * @param blockIndex block index
 */
void UniformBlock::dcbz(s32 blockIndex) const
{
    std::memset(static_cast<u8*>(mCurrentBuffer) + mAlignedBlockSize * blockIndex, 0,
                mBlockSize);
}

/**
 * Clears a block of the current buffer.
 * @param unused unused
 * @param blockIndex block index
 */
void UniformBlock::dcbz(u32 unused, s32 blockIndex) const
{
    std::memset(static_cast<u8*>(mCurrentBuffer) + mAlignedBlockSize * blockIndex, 0,
                mBlockSize);
}

}  // namespace agl
