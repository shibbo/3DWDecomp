#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class BreakModel;
class HitSensor;
class MtxConnector;
}  // namespace al

class BlockHardLaserOnlyDebris;
class DisasterBlockDirector;

/**
 * @brief Hard block in Bowser's Fury that only breaks to Fury Bowser's laser, and glows while
 * disaster mode is active.
 */
class BlockHardLaserOnly : public al::LiveActor {
public:
    /** @brief Which break model (and size) this block uses. */
    enum class BreakModelType : s32 { Size2x2 = 0, Size4x2 = 1, Size6x2 = 2, Size8x2 = 3 };

    explicit BlockHardLaserOnly(const char* pName);
    ~BlockHardLaserOnly() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appearDebris();
    void control() override;
    void kill() override;
    void startClipped() override;
    void endClipped() override;
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void breakBlock(s32 delay);
    void exeWait();
    bool isDisabled() const;
    bool isDisaster();
    bool isPlayerInFullGlowRange();
    void exeReaction();
    void exeBreakStart();
    void exeBreaking();
    void exeFullGlowDelay();
    bool isGlowAnimDone();
    void exeFullGlowOn();
    void exeFullGlow();
    bool isEqualDirectorSoundPlayer();
    void exeFullGlowOff();
    s32 getFileID();
    bool canChainBreak();
    f32 getMaxChainBreakDistance() const;
    bool isBreaking(bool isCheckDelay);

private:
    DisasterBlockDirector* mDirector = nullptr;
    void* _150;
    al::BreakModel* mBreakModel = nullptr;
    al::HitSensor* mAttacker = nullptr;
    al::MtxConnector* mConnector = nullptr;
    s32 mSaveId = 0;
    s32 mFileID = -1;
    s32 mBreakDelay = 10;
    bool mCanChainBreak = true;
    f32 mMaxChainBreakDistance = -1.0f;
    bool mIsDisasterMode = false;
    BreakModelType mBreakModelType = BreakModelType::Size2x2;
    BlockHardLaserOnlyDebris* mDebris[2] = {nullptr, nullptr};
    bool mIsDisabledInPhase0;
};

static_assert(sizeof(BlockHardLaserOnly) == 0x1a8);
