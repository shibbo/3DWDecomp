#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class BgmRhythmAnimeController;
class FlowerRhythm : public al::LiveActor {
public:
    explicit FlowerRhythm(const char*);
    ~FlowerRhythm() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    void destroy(const al::SensorMsg*, const al::HitSensor*);
    void tryAppearItem(const al::SensorMsg*, const al::HitSensor*, bool, bool);
    void tryAppearItemScreenPointer(const al::SensorMsg*, const al::ScreenPointer*);
    void exeWait();
    void exeReaction();
    void exeReactionAttack();
private:
    al::LiveActor* mTrace;
    int mReactionCooldown = 0;
    const char* mItemType = nullptr;
    BgmRhythmAnimeController* mRhythm = nullptr;
    bool mCottonGone = true;
    bool mIsPaw = false;
    al::MtxConnector* mConnector = nullptr;
};
static_assert(sizeof(FlowerRhythm) == 0x178);
