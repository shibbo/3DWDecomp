/**
 * @file TexCoordArray.h
 * @brief Texture coordinate array implementation.
 */

#pragma once

#include <nn/types.h>

namespace nn {

namespace util {
struct Float2;
}

namespace ui2d {
class Layout;

namespace detail {
class TexCoordArray {
public:
    void Initialize();
    void Free();
    void Reserve(s32);
    void SetSize(s32 size);
    void GetCoord(nn::util::Float2*, s32) const;
    void SetCoord(s32, nn::util::Float2 const*);
    void Copy(void const*, s32);
    bool CompareCopiedInstanceTest(nn::ui2d::detail::TexCoordArray const&) const;

    /** @return The number of coordinate sets the array can hold. */
    u8 GetCapacity() const { return mCapacity; }
    /** @return The number of coordinate sets in use. */
    u8 GetSize() const { return mSize; }
    /** @return The coordinate sets, four corners each. */
    const nn::util::Float2 (*GetArray() const)[4] { return mCoords; }

    u8 mCapacity;
    u8 mSize;
    nn::util::Float2 (*mCoords)[4];
};
}  // namespace detail
}  // namespace ui2d
}  // namespace nn
