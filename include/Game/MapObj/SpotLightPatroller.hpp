#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <common/aglTextureData.h>
#include <gfx/seadColor.h>
class SpotLightPatroller : public al::LiveActor {
public:
    SpotLightPatroller(const char*);
    ~SpotLightPatroller() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void movementPaused(bool) override;
    void killBySwitch();
    void exePatrol();
    void exeAlert();
    void setPatrol();
    void setAlert();
    bool isPlayerWatch() const;
    bool isAlert() const;
    void setObserved(bool observed) { mIsObserved = observed; }
private:
    float mSpeed = 5.0f;
    agl::TextureData mTexture;
    void* mTextureResource = nullptr;
    float mRadius = 0.0f;
    int mEffectType = 0;
    sead::Matrix34f mEffectMtx = sead::Matrix34f::ident;
    int mHitCheckType = 0;
    sead::Color4f mColor;
    sead::Vector4f mUnused;
    int mLppType = 0;
    bool mIsObserved = false;
    bool mIsCheckCollision = false;
    int mLightState = -1;
};
static_assert(sizeof(SpotLightPatroller) == 0x2e0);
