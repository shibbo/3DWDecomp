#pragma once

#include <nn/ui2d/ui2d_Types.h>

namespace nn::ui2d {
class TextureInfo;

class TexMap {
public:
    TexMap();
    explicit TexMap(const TextureInfo* pTextureInfo);
    ~TexMap();
    void Finalize();
    void ResetSamplerSettings();
    void ResetTextureInfoState();
    void Set(const TextureInfo* pTextureInfo);
    void SetWrapMode(TexWrap wrapS, TexWrap wrapT);
    void SetFilter(TexFilter minFilter, TexFilter magFilter);
    void CopySamplerSettings(const TexMap& rOther);

    union {
        struct {
            u16 mWrapS : 2;
            u16 mWrapT : 2;
            u16 mMinFilter : 3;
            u16 mMagFilter : 1;
            u16 mTextureInfoState : 5;
            u16 mReserved : 3;
        };
        struct {
            u16 : 8;
            u16 mIsDynamicTextureOwner : 1;
            u16 mShareInfoStackOffset : 3;
            u16 mIsDummyTextureInfo : 1;
            u16 : 3;
        };
    };
    const TextureInfo* m_pTextureInfo;
};

static_assert(sizeof(TexMap) == 0x10, "TexMap size");
}  // namespace nn::ui2d
