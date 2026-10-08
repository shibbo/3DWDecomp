#pragma once

#include <basis/seadTypes.h>

#include "Player/PlayerAction.hpp"

class IUsePlayerCheckArea;
class IUsePlayerFlagControl;
class IUsePlayerForwardBent;
class IUsePlayerWaterFlowField;
class PlayerTrigger;
struct PlayerActionArg;

/// The player's stand swim action: swimming upright underwater, paddling up and walking on the
/// bottom.
class PlayerActionStandSwim : public PlayerAction {
    SEAD_RTTI_OVERRIDE(PlayerActionStandSwim, PlayerAction)

public:
    PlayerActionStandSwim(const PlayerActionArg* pArg, const PlayerTrigger* pTrigger,
                          const IUsePlayerWaterFlowField* pWaterFlowField,
                          IUsePlayerForwardBent* pForwardBent, IUsePlayerFlagControl* pFlagControl,
                          const IUsePlayerCheckArea* pCheckArea);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

    void updateVelocity();
    bool isSwimTramplePlaying();
    void controlAnim();
    void calcForwardBent();

protected:
    const PlayerActionArg* mArg;                        // 0x8
    const PlayerTrigger* mTrigger;                      // 0x10
    const IUsePlayerWaterFlowField* mWaterFlowField;    // 0x18
    const IUsePlayerCheckArea* mCheckArea;              // 0x20
    IUsePlayerForwardBent* mForwardBent;                // 0x28
    u32 mHighAccelFrame = 0;                            // 0x30, frames the high acceleration lasts
    u32 _34 = 0;                                        // 0x34
    bool mIsOnFloorPrev = false;                        // 0x38, on the floor last frame
    u32 mMoveAnimKeepFrame = 0;                         // 0x3c, frames SwimStandMove is kept
    const char* mStartAnimName = nullptr;               // 0x40, animation not interrupted by setup
    IUsePlayerFlagControl* mFlagControl;                // 0x48
    u32 mFlagFrame = 0;                                 // 0x50, frames until the flag turns off
    u32 mPaddleFrame = 0;                               // 0x54, frames since the last paddle
    f32 mPaddleAnimRate = 1.0f;                         // 0x58
    bool mIsPaddle = false;                             // 0x5c, a paddle animation is requested
    bool mIsTrampleStart = false;                       // 0x5d
    u32 mFromDiveFrame = 0;                             // 0x60, frames since the dive ended
    bool mIsInWaterNoSink = false;                      // 0x64
};
