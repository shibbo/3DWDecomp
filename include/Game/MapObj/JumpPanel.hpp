#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
namespace al { class MtxConnector; class CollisionObj; }
class JumpPanel : public al::LiveActor {
public:
    explicit JumpPanel(const char*);
    ~JumpPanel() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    bool hideActor() override;
    bool showActor() override;
    void appear() override;
    void makeActorDead() override;
    void control() override;
    void movementPaused(bool) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void onPanelAppear();
    void offPanelAppear();
    void exeWait();
    void exeTouchWait();
    void exeJump();
    void exeBindWait();
private:
    sead::FixedPtrArray<al::HitSensor, 16> mPlayers;
    float mJumpPower = 42.0f;
    al::MtxConnector* mConnector = nullptr;
    al::CollisionObj* mCollision = nullptr;
    bool mTouchAssist = false;
    bool mIsSuper = false;
    bool mAttachOnAppear = false;
    bool mIsSingleMode = false;
    s32 mJumpInputFrames = 0;
    s32 mTouchFrames = 0;
    s32 mTouchReleaseFrames = 0;
    bool mKoopaJrHipDrop = false;
};
static_assert(sizeof(JumpPanel) == 0x208);
