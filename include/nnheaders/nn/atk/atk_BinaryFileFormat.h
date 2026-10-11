#pragma once
#include <nn/types.h>

namespace nn::atk::detail {
struct Reference { u16 type, reserved; s32 offset; };
struct ReferenceWithSize { u16 type, reserved; u32 offset, size; };
struct BinaryFileHeader {
    u32 signature;
    u16 byteOrder, headerSize;
    u32 version, fileSize;
    u16 blockCount, reserved;
};
static_assert(sizeof(BinaryFileHeader) == 0x14, "BinaryFileHeader size");
struct BinaryBlockHeader {
    u32 kind;
    u32 size;
};
static_assert(sizeof(ReferenceWithSize) == 0xc, "ReferenceWithSize size");
}
