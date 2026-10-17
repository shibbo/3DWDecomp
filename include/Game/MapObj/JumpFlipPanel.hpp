#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class CollisionObj; }
class JumpFlipPanel : public al::LiveActor {
public:
    explicit JumpFlipPanel(const char*);
    ~JumpFlipPanel() override;
    void init(const al::ActorInitInfo&) override;
    bool isFarLodSwitchOkay() override;
    void start();
    void updateLodAnim();
    void exeWait();
    void exeFlip();
    bool isFlipping();
    void exeSwitchOffStart();
private:
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    float mRotation = 0.0f;
    bool mIsFront = true;
    al::CollisionObj* mCollision = nullptr;
    sead::Matrix34f mCollisionMtx = sead::Matrix34f::ident;
    bool mIsUpperTurn = false;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    bool mIsSingleMode = false;
};
static_assert(sizeof(JumpFlipPanel) == 0x1b0);
