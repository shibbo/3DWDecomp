#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}

/**
 * One shell-fur shape drawn on top of an actor's model.
 */
class FurShape {
public:
    FurShape(al::LiveActor* pActor, const char* pShapeName, const char* pDiameterTextureName);

    void draw() const;
    void updateUbo();

    void setLayerNum(s32 layerNum) { mLayerNum = layerNum; }
    void setHeight(f32 height) { mHeight = height; }
    void setAlpha(f32 startAlpha, f32 endAlpha) {
        mStartAlpha = startAlpha;
        mEndAlpha = endAlpha;
    }
    void setUnk14(bool value) { _14 = value; }

private:
    al::LiveActor* mActor;
    void* _8;
    s32 _10;
    bool _14;
    f32 mHeight;
    f32 mStartAlpha;
    f32 mEndAlpha;
    s32 mLayerNum;
};

static_assert(sizeof(FurShape) == 0x28);
