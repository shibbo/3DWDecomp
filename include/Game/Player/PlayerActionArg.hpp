#pragma once

class IUsePlayerAnimator;
class IUsePlayerAudio;
class IUsePlayerCollision;
class IUsePlayerEffect;
class IUsePlayerEventReceiver;
class IUsePlayerHorizontalSpeedAverage;
class IUsePlayerInput;
class IUsePlayerReaction;
class IUsePlayerSubAction;
class PlayerConstParam;
struct PlayerProperty;

/// The player systems every action gets handed.
struct PlayerActionArg {
    const IUsePlayerInput* getInput() const { return mInput; }

    PlayerProperty* mProperty;                // 0x0
    IUsePlayerAnimator* mAnimator;            // 0x8
    IUsePlayerEffect* mEffect;                // 0x10
    IUsePlayerAudio* mAudio;                  // 0x18
    IUsePlayerCollision* mCollision;          // 0x20
    unsigned char _28[0x30 - 0x28];
    const IUsePlayerInput* mInput;            // 0x30
    IUsePlayerHorizontalSpeedAverage* mHorizontalSpeedAverage;  // 0x38
    IUsePlayerSubAction* mSubAction;          // 0x40
    IUsePlayerReaction* mReaction;            // 0x48
    IUsePlayerEventReceiver* mEventReceiver;  // 0x50
    const PlayerConstParam* mConstParam;      // 0x58
};
