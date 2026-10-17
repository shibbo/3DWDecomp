#pragma once

#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class BgmRhythmAnimeController;

/**
 * @brief A bush that shakes when hit and can hide an item or a rabbit.
 */
class Bush : public al::LiveActor {
public:
    explicit Bush(const char* pName);
    ~Bush() override;
    void init(const al::ActorInitInfo&) override;
    void initWithColorFrame(const al::ActorInitInfo&, int, const char*);
    void initAfterPlacement() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool trySetAppearItemFactorByMsg(const al::SensorMsg*, const al::HitSensor*, const char*);
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    void exeWait();
    void exeReaction();
    bool tryAppearItem();
    void exeReactionAttack();
    void exeReactionFire();
    void exeReactionDeath();
    void exeReactionMic();
    void exeReactionTouch();
    void exeRabbitWait();
    void exeRabbitAppear();

    void initBushOnRabbit(const al::ActorInitInfo& rInfo, const sead::Vector3f& rTrans,
                          s32 color);
    bool tryStartRabbitAppear();

private:
    al::MtxConnector* mConnector = nullptr;
    int mReactionCooldown = 0;
    int mMicReleaseFrames = 0;
    const char* mItemType = nullptr;
    bool mIsBush = false;
    bool mRabbitAppeared = true;
    bool mIsSingleMode = false;
    BgmRhythmAnimeController* mRhythm = nullptr;
};

static_assert(sizeof(Bush) == 0x170);
