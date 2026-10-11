#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

namespace nn::ui2d {
class TextureInfo;
}  // namespace nn::ui2d

namespace rc {
class Stamp;
class StampDirector;
}  // namespace rc

/**
 * @brief Layout of the snapshot mode: stamp preview, filter name, logo and the button guide.
 */
class SnapshotLayout : public al::LayoutActor {
public:
    /** @brief Logo shown on the snapshot. */
    enum LogoType : u8 {
        LogoType_None = 0,
        LogoType_3DWorld = 1,
        LogoType_SingleMode = 2,
        LogoType_Combo = 3,
    };

    SnapshotLayout(const al::LayoutInitInfo& rInfo, rc::StampDirector* pStampDirector);

    void appear() override;
    void setStartingStampTexture();
    void end();
    void updateFilterText(s32 filterIndex);
    void updatePreviewImage(rc::Stamp* pStamp);
    void rotatePreviewImage(f32 degree);
    void incLogoType();
    void decLogoType();
    void updateLogoAngle(f32 angle);
    void toggleInfoDisplay(bool isUserToggle);

    void exeAppear();
    void exeWait();
    void exeEnd();

private:
    void updateGuideMessage();
    void setPreviewTextureSmall();
    void setPreviewTextureLarge();

    bool mIsWaitFirstInput = true;              // 0x121
    bool mIsInfoDisplay = true;                 // 0x122
    bool mIsInfoHiddenByUser = false;           // 0x123
    s32 mInfoDisplayTimer = 120;                // 0x124
    nn::ui2d::TextureInfo* mTextureInfo = nullptr;  // 0x128
    rc::StampDirector* mStampDirector;          // 0x130
    void* _138 = nullptr;                       // 0x138
    LogoType mLogoType = LogoType_None;         // 0x140
};

static_assert(sizeof(SnapshotLayout) == 0x148);
