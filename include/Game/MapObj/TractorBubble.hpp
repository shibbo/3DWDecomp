#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "utility/aglParameterObj.h"

namespace al {
class ActorInitInfo;
class HitSensor;
class SensorMsg;
}  // namespace al

class TractorBubblePuppeteer;

/// Bubble that carries a player who fell behind back to the others.
class TractorBubble : public al::LiveActor {
public:
    TractorBubble(bool isSingleMode);

    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void receiveBindInit(al::HitSensor* pSelf, al::HitSensor* pOther);
    void receiveBindCancel();

    void activatePlayerWithBubble(al::LiveActor* pPlayer);
    void startBubbleWithScreenOut(al::LiveActor* pPlayer);
    void startBubbleWithInput(al::LiveActor* pPlayer);
    bool isEnableBubbleOutFrame(const al::LiveActor* pPlayer);
    bool isEnableBubbleInput(const al::LiveActor* pPlayer);
    bool isPlayerInBubble();
    bool isBubble();
    bool isBindWait();
    void updateShadow();
    void cancelBubble();

    void exeWait();
    void exeBindWait();
    void exeAppearBubble();
    void exeBubble();
    void exeBurstBubble();

private:
    TractorBubblePuppeteer* mPuppeteer = nullptr;  // 0x148
    const al::LiveActor* mTargetPlayer = nullptr;  // 0x150
    sead::Vector3f mBubblePos = {0.0f, 0.0f, 0.0f};  // 0x158
    f32 mSpeed = 0.0f;  // 0x164
    f32 mWaveFrame = 0.0f;  // 0x168
    bool mIsEnableCancel = true;  // 0x16c
    u8 _16d;
    bool mIsActivate = false;  // 0x16e
    bool _16f = true;  // 0x16f
    s32 _170 = 0;  // 0x170
    f32 mHitWallRate = 0.0f;  // 0x174
    sead::Vector3f mPushOffset = {0.0f, 0.0f, 0.0f};  // 0x178
    void* _188 = nullptr;  // 0x188
    sead::Vector3f mPrevGoalPos;  // 0x190
    sead::Vector3f mGoalPos;  // 0x19c
    sead::Vector3f mSmoothGoalPos;  // 0x1a8
    agl::utl::ParameterObj mParameterObj;  // 0x1b8
    bool mIsShadowInArea = false;  // 0x1e8
    bool mIsSingleMode;  // 0x1e9
    f32 mShadowDropLength = 1000.0f;  // 0x1ec
};

static_assert(sizeof(TractorBubble) == 0x1f0);
