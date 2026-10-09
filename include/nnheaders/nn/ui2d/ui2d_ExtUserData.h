#pragma once

#include <nn/types.h>

namespace nn::ui2d {

struct ResExtUserData {
    u32 nameOffset;
    u32 dataOffset;
    u16 count;
    u8 type;
    u8 reserved;

    const void* GetData() const {
        return reinterpret_cast<const char*>(this) + dataOffset;
    }

    /** @return The name of the data, or nullptr when it has none. */
    const char* GetName() const {
        return nameOffset != 0 ? reinterpret_cast<const char*>(this) + nameOffset : nullptr;
    }

    /** @return The data as an array of integers. */
    const s32* GetIntArray() const { return static_cast<const s32*>(GetData()); }

    /** @return The data as an array of floats. */
    const float* GetFloatArray() const { return static_cast<const float*>(GetData()); }
};

}  // namespace nn::ui2d
