#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BgmBeatAnimeController;
class BlockSlot : public al::LiveActor {
public:
    explicit BlockSlot(const char*);
    ~BlockSlot() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void control() override;
    void exeWait();
    void exeStart();
    void exeRolling();
    void exeReaction();
    void exeStop();
    void startRolling();
    bool isStop() const;
    int getResultNumber() const;
    void startPerfect();
    void tryStartMatch();
    int getMatchIndex() const { return mMatchIndex; }
    void setMatchIndex(int index) { mMatchIndex = index; }
private:
    al::LiveActor* mReel = nullptr;
    float mReelFrame = 0.0f;
    int mSymbolCount = 5;
    float mRollingSpeed = 0.0f;
    int mMatchIndex = -1;
    float mTargetFrame = 0.0f;
    BgmBeatAnimeController* mBeatController = nullptr;
};
static_assert(sizeof(BlockSlot) == 0x170);
